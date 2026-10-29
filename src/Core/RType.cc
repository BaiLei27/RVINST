#include <string>

#include "Core/RType.hh"
#include "ISA/Regs.hh"

namespace {

constexpr uint8_t G_RM_DYN= 7;

[[nodiscard]] bool nameStartsWith(std::string_view name, std::string_view prefix)
{
    return name.starts_with(prefix);
}

[[nodiscard]] bool nameContains(std::string_view name, std::string_view needle)
{
    return std::string_view::npos != name.find(needle);
}

// OP-FP: rs2 encodes a variant (fsqrt/fcvt/fmv/fclass), not a source operand.
[[nodiscard]] bool fpRs2IsSelect(uint8_t f7) noexcept
{
    switch(f7 & ~0x3U) {
    case 0x20: // fcvt between FP formats
    case 0x2C: // fsqrt
    case 0x60: // fcvt to int
    case 0x68: // fcvt from int
    case 0x70: // fmv.x.* / fclass
    case 0x78: // fmv.*.x
        return true;
    default: return false;
    }
}

enum class FpOperandKind : uint8_t {
    BIN_FP,    // rd,rs1,rs2 all f
    UNARY_FP,  // rd,rs1 f (fsqrt)
    FP_TO_INT, // rd x, rs1 f
    INT_TO_FP, // rd f, rs1 x
    CMP,       // rd x, rs1/rs2 f
    R4,        // rd,rs1,rs2,rs3 f
    INT_ALU,   // integer R
};

[[nodiscard]] FpOperandKind classifyFpName(std::string_view name) noexcept
{
    if(nameStartsWith(name, "fmadd")
       || nameStartsWith(name, "fmsub")
       || nameStartsWith(name, "fnmadd")
       || nameStartsWith(name, "fnmsub")) {
        return FpOperandKind::R4;
    }
    if(nameStartsWith(name, "fsqrt")) return FpOperandKind::UNARY_FP;
    if(nameStartsWith(name, "feq") || nameStartsWith(name, "flt") || nameStartsWith(name, "fle")) {
        return FpOperandKind::CMP;
    }
    if(nameStartsWith(name, "fclass") || nameStartsWith(name, "fmv.x.")) return FpOperandKind::FP_TO_INT;
    if(nameStartsWith(name, "fmv.") && nameContains(name, ".x")) return FpOperandKind::INT_TO_FP;
    if(nameStartsWith(name, "fcvt.")) {
        // fcvt.<int>.<fp> ? FP_TO_INT; fcvt.<fp>.<int> ? INT_TO_FP; fcvt.<fp>.<fp> ? UNARY_FP
        if(nameContains(name, ".w.") || nameContains(name, ".wu.") || nameContains(name, ".l.")
           || nameContains(name, ".lu.")) {
            return FpOperandKind::FP_TO_INT;
        }
        if(name.ends_with(".w") || name.ends_with(".wu") || name.ends_with(".l") || name.ends_with(".lu")) {
            return FpOperandKind::INT_TO_FP;
        }
        return FpOperandKind::UNARY_FP;
    }
    if(nameStartsWith(name, "f")) return FpOperandKind::BIN_FP;

    return FpOperandKind::INT_ALU;
}

} // namespace

RType::RType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
}

RType::RType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(format, std::move(instAssembly), hasSetABI)
{
    init();
}

bool RType::isAmoName(std::string_view name) noexcept
{
    return nameStartsWith(name, "amo") || nameStartsWith(name, "lr.") || nameStartsWith(name, "sc.");
}

bool RType::isR4Opc(uint8_t opc) noexcept
{
    return OpcMap::MADD == opc || OpcMap::MSUB == opc || OpcMap::NMSUB == opc || OpcMap::NMADD == opc;
}

bool RType::fpUsesRm(uint8_t f7) noexcept
{
    switch(f7 & ~0x3U) {
    case 0x00: // fadd
    case 0x04: // fsub
    case 0x08: // fmul
    case 0x0C: // fdiv
    case 0x20: // fcvt fmt
    case 0x2C: // fsqrt
    case 0x60: // fcvt to int
    case 0x68: // fcvt from int
        return true;
    default: return false;
    }
}

void RType::Parse()
{
    switch(DetectRShape(Layout_.R.opc)) {
    case RShape::AMO:
        setBitsField({ Layout_.A.opc,
                       Layout_.A.rd,
                       Layout_.A.fct3,
                       Layout_.A.rs1,
                       Layout_.A.rs2,
                       Layout_.A.rl,
                       Layout_.A.aq,
                       Layout_.A.fct5 });
        break;
    case RShape::FP:
        setBitsField({ Layout_.FP.opc,
                       Layout_.FP.rd,
                       Layout_.FP.rm,
                       Layout_.FP.rs1,
                       Layout_.FP.rs2,
                       Layout_.FP.fmt,
                       Layout_.FP.fct5 });
        break;
    case RShape::R4:
        setBitsField({ Layout_.R4.opc,
                       Layout_.R4.rd,
                       Layout_.R4.rm,
                       Layout_.R4.rs1,
                       Layout_.R4.rs2,
                       Layout_.R4.fmt,
                       Layout_.R4.rs3 });
        break;
    default:
        setBitsField({ Layout_.R.opc,
                       Layout_.R.rd,
                       Layout_.R.fct3,
                       Layout_.R.rs1,
                       Layout_.R.rs2,
                       Layout_.R.fct7 });
        break;
    }
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nfunct7: {}\nrs1: {}\nrs2: {}\nrd: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.R.fct3,
                 +Layout_.R.fct7,
                 +Layout_.R.rs1,
                 +Layout_.R.rs2,
                 +Layout_.R.rd);
#endif
}

void RType::mnemonicHelper()
{
    const auto NAME= !InstAssembly_.empty() ? std::string_view { InstAssembly_[0] }
                                            : LookupNameAndInfo().name_;

    if(OpcMap::AMO == Layout_.R.opc || isAmoName(NAME)) {
        auto rd = isa::LookupRegName(Layout_.A.rd, HasSetABI_);
        auto rs1= isa::LookupRegName(Layout_.A.rs1, HasSetABI_);
        if(nameStartsWith(NAME, "lr.")) {
            appendOperands({ " ", rd, ",", "(", rs1, ")" });
        } else {
            auto rs2= isa::LookupRegName(Layout_.A.rs2, HasSetABI_);
            appendOperands({ " ", rd, ",", rs2, ",", "(", rs1, ")" });
        }
        if(Layout_.A.aq || Layout_.A.rl) {
            std::string tagged { NAME };
            if(!tagged.ends_with(".aq") && !tagged.ends_with(".rl")) {
                if(Layout_.A.aq) tagged+= ".aq";
                if(Layout_.A.rl) tagged+= ".rl";
            }
            if(!InstAssembly_.empty()) InstAssembly_[0]= std::move(tagged);
        }
        return;
    }

    const auto KIND= classifyFpName(NAME);
    switch(KIND) {
    case FpOperandKind::R4: {
        auto rd = isa::LookupFregName(Layout_.R4.rd, HasSetABI_);
        auto rs1= isa::LookupFregName(Layout_.R4.rs1, HasSetABI_);
        auto rs2= isa::LookupFregName(Layout_.R4.rs2, HasSetABI_);
        auto rs3= isa::LookupFregName(Layout_.R4.rs3, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1, ",", rs2, ",", rs3 });
        return;
    }
    case FpOperandKind::BIN_FP: {
        auto rd = isa::LookupFregName(Layout_.R.rd, HasSetABI_);
        auto rs1= isa::LookupFregName(Layout_.R.rs1, HasSetABI_);
        auto rs2= isa::LookupFregName(Layout_.R.rs2, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1, ",", rs2 });
        return;
    }
    case FpOperandKind::UNARY_FP: {
        auto rd = isa::LookupFregName(Layout_.R.rd, HasSetABI_);
        auto rs1= isa::LookupFregName(Layout_.R.rs1, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1 });
        return;
    }
    case FpOperandKind::FP_TO_INT: {
        auto rd = isa::LookupRegName(Layout_.R.rd, HasSetABI_);
        auto rs1= isa::LookupFregName(Layout_.R.rs1, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1 });
        return;
    }
    case FpOperandKind::INT_TO_FP: {
        auto rd = isa::LookupFregName(Layout_.R.rd, HasSetABI_);
        auto rs1= isa::LookupRegName(Layout_.R.rs1, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1 });
        return;
    }
    case FpOperandKind::CMP: {
        auto rd = isa::LookupRegName(Layout_.R.rd, HasSetABI_);
        auto rs1= isa::LookupFregName(Layout_.R.rs1, HasSetABI_);
        auto rs2= isa::LookupFregName(Layout_.R.rs2, HasSetABI_);
        appendOperands({ " ", rd, ",", rs1, ",", rs2 });
        return;
    }
    case FpOperandKind::INT_ALU:
    default:                     break;
    }

    auto rd = isa::LookupRegName(Layout_.R.rd, HasSetABI_);
    auto rs1= isa::LookupRegName(Layout_.R.rs1, HasSetABI_);
    auto rs2= isa::LookupRegName(Layout_.R.rs2, HasSetABI_);
    appendOperands({ " ", rd, ",", rs1, ",", rs2 });
}

util::Expected_u<const InstLayout *> RType::Assembly()
{
    const auto &info  = LookupIdxAndInfo();
    Layout_.R.opc     = info.opcode_;
    const auto PAYLOAD= KEY_PAYLOAD(info.keyID_);
    const auto NAME   = InstAssembly_.empty() ? std::string_view {} : std::string_view { InstAssembly_[0] };

    if(OpcMap::AMO == info.opcode_) {
        Layout_.A.fct5= PAYLOAD >> 3;
        Layout_.A.fct3= PAYLOAD;
        Layout_.A.aq  = 0;
        Layout_.A.rl  = 0;

        if(InstAssembly_.size() >= 3U) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) Layout_.A.rd= *rd;
            if(nameStartsWith(NAME, "lr.")) {
                std::string_view base= InstAssembly_.at(2);
                if(base.starts_with('(') && base.ends_with(')')) {
                    base= base.substr(1, base.size() - 2);
                }
                if(auto rs1= isa::LookupRegIdx(base)) Layout_.A.rs1= *rs1;
                Layout_.A.rs2= 0;
            } else if(InstAssembly_.size() >= 4U) {
                if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(2))) Layout_.A.rs2= *rs2;
                std::string_view base= InstAssembly_.at(3);
                if(base.starts_with('(') && base.ends_with(')')) {
                    base= base.substr(1, base.size() - 2);
                }
                if(auto rs1= isa::LookupRegIdx(base)) Layout_.A.rs1= *rs1;
            }
        }
        mnemonicHelper();
        return &Layout_;
    }

    if(isR4Opc(info.opcode_)) {
        Layout_.R4.fmt= PAYLOAD & 0x3U;
        Layout_.R4.rm = G_RM_DYN;
        if(InstAssembly_.size() >= 5U) {
            if(auto rd= isa::LookupFregIdx(InstAssembly_.at(1))) Layout_.R4.rd= *rd;
            if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R4.rs1= *rs1;
            if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(3))) Layout_.R4.rs2= *rs2;
            if(auto rs3= isa::LookupFregIdx(InstAssembly_.at(4))) Layout_.R4.rs3= *rs3;
        }
        mnemonicHelper();
        return &Layout_;
    }

    if(OpcMap::OP_FP == info.opcode_) {
        const uint8_t F7 = static_cast<uint8_t>(PAYLOAD >> 8);
        const uint8_t RS2= static_cast<uint8_t>(PAYLOAD >> 3 & 0x1FU);
        const uint8_t F3 = static_cast<uint8_t>(PAYLOAD & 7U);
        Layout_.R.fct7   = F7;
        Layout_.R.rs2    = RS2;
        Layout_.R.fct3   = fpUsesRm(F7) ? G_RM_DYN : F3;

        const auto KIND= classifyFpName(NAME);
        switch(KIND) {
        case FpOperandKind::BIN_FP:
            if(InstAssembly_.size() >= 4U) {
                if(auto rd= isa::LookupFregIdx(InstAssembly_.at(1))) Layout_.R.rd= *rd;
                if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R.rs1= *rs1;
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(3))) Layout_.R.rs2= *rs2;
            }
            break;
        case FpOperandKind::UNARY_FP:
            if(InstAssembly_.size() >= 3U) {
                if(auto rd= isa::LookupFregIdx(InstAssembly_.at(1))) Layout_.R.rd= *rd;
                if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R.rs1= *rs1;
            }
            break;
        case FpOperandKind::FP_TO_INT:
            if(InstAssembly_.size() >= 3U) {
                if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) Layout_.R.rd= *rd;
                if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R.rs1= *rs1;
            }
            break;
        case FpOperandKind::INT_TO_FP:
            if(InstAssembly_.size() >= 3U) {
                if(auto rd= isa::LookupFregIdx(InstAssembly_.at(1))) Layout_.R.rd= *rd;
                if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(2))) Layout_.R.rs1= *rs1;
            }
            break;
        case FpOperandKind::CMP:
            if(InstAssembly_.size() >= 4U) {
                if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) Layout_.R.rd= *rd;
                if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R.rs1= *rs1;
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(3))) Layout_.R.rs2= *rs2;
            }
            break;
        default: break;
        }
        mnemonicHelper();
        return &Layout_;
    }

    Layout_.R.fct7= PAYLOAD >> 3;
    Layout_.R.fct3= PAYLOAD;

    if(InstAssembly_.size() >= 4U) {
        auto rdOpt = isa::LookupRegIdx(InstAssembly_.at(1));
        auto rs1Opt= isa::LookupRegIdx(InstAssembly_.at(2));
        auto rs2Opt= isa::LookupRegIdx(InstAssembly_.at(3));
        if(rdOpt && rs1Opt && rs2Opt) {
            Layout_.R.rd = *rdOpt;
            Layout_.R.rs1= *rs1Opt;
            Layout_.R.rs2= *rs2Opt;
        }
    }

    mnemonicHelper();
    return &Layout_;
}

auto RType::calculateKeyID() -> KeyT
{
    if(OpcMap::AMO == Layout_.R.opc) {
        return Key_= AMO_K(Layout_.A.fct5, Layout_.A.fct3, Layout_.A.opc);
    }
    if(isR4Opc(Layout_.R.opc)) {
        return Key_= R4_K(Layout_.R4.fmt, Layout_.R4.opc);
    }
    if(OpcMap::OP_FP == Layout_.R.opc) {
        const uint8_t F7   = Layout_.R.fct7;
        const uint8_t RS2  = fpRs2IsSelect(F7) ? Layout_.R.rs2 : 0;
        const uint8_t F3KEY= fpUsesRm(F7) ? 0 : Layout_.R.fct3;
        return Key_        = FP_K(F7, RS2, F3KEY);
    }
    return Key_= OP_K(Layout_.R.fct7, Layout_.R.fct3, Layout_.R.opc);
}

auto RType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
