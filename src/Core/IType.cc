#include "Core/IType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hh"

namespace {

constexpr uint8_t G_FENCE_IORW= 0xFU; // I|O|R|W

std::string formatFenceSet(uint16_t mask)
{
    std::string s;
    // NOLINTBEGIN

    if(mask & 0x8U) s+= 'i';
    if(mask & 0x4U) s+= 'o';
    if(mask & 0x2U) s+= 'r';
    if(mask & 0x1U) s+= 'w';

    // NOLINTEND
    return s.empty() ? "0" : s;
}

std::optional<uint16_t> parseFenceSet(std::string_view tok)
{
    if(tok.empty()) return std::nullopt;

    uint16_t num {};
    if(util::ParseInt(tok, num)) {
        if(num > 15U) return std::nullopt;
        return num;
    }

    uint16_t mask {};
    for(unsigned char c: tok) {
        switch(std::tolower(c)) {
        case 'i': mask|= 0x8U; break;
        case 'o': mask|= 0x4U; break;
        case 'r': mask|= 0x2U; break;
        case 'w': mask|= 0x1U; break;
        default:  return std::nullopt;
        }
    }
    return mask;
}

constexpr uint16_t PACK_FENCE_IMM(uint16_t fm, uint16_t pred, uint16_t succ) noexcept
{
    return (fm & 0xFU) << 8 | (pred & 0xFU) << 4 | (succ & 0xFU);
}

} // namespace

IType::IType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
}

IType::IType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(format, std::move(instAssembly), hasSetABI)
{
    init();
}

void IType::Parse()
{
    const auto IMM= Layout_.I.imm0tB;
    switch(DetectIImmShape(Layout_.I.opc, Layout_.I.fct3)) {
    case ItypeImmShape::SHIFT:
        setBitsField({ Layout_.I.opc, Layout_.I.rd, Layout_.I.fct3, Layout_.I.rs1, IMM & 0x1FU, IMM >> 5 });
        break;
    case ItypeImmShape::FENCE:
        setBitsField({ Layout_.I.opc,
                       Layout_.I.rd,
                       Layout_.I.fct3,
                       Layout_.I.rs1,
                       IMM & 0xFU,
                       IMM >> 4 & 0xFU,
                       IMM >> 8 & 0xFU });
        break;
    default:
        setBitsField({ Layout_.I.opc, Layout_.I.rd, Layout_.I.fct3, Layout_.I.rs1, IMM });
        break;
    }
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrd: {}\nimm: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.I.fct3,
                 +Layout_.I.rs1,
                 +Layout_.I.rd,
                 +Layout_.I.imm0tB);
#endif
}

void IType::mnemonicHelper()
{
    const uint32_t OPC= Layout_.I.opc;
    const auto SHAPE  = DetectIImmShape(OPC, Layout_.I.fct3);

    if(OpcMap::SYSTEM == OPC) {
        if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
        return;
    }

    if(ItypeImmShape::FENCE == SHAPE) {
        if(1 == Layout_.I.fct3) { // fence.i
            if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
            return;
        }
        const auto PRED= formatFenceSet(Layout_.I.imm0tB >> 4 & 0xFU);
        const auto SUCC= formatFenceSet(Layout_.I.imm0tB & 0xFU);
        appendOperands({ " ", PRED, ",", SUCC });
        return;
    }

    auto rd           = isa::LOOKUP_REG_NAME(Layout_.I.rd, HasSetABI_);
    auto rs1          = isa::LOOKUP_REG_NAME(Layout_.I.rs1, HasSetABI_);
    std::string immStr= std::to_string(Layout_.I.imm0tB);
    appendOperands({ " ", rd, ",", rs1, ",", immStr });
}

util::Expected_u<const InstLayout *> IType::Assembly()
{
    const auto &info  = LookupIdxAndInfo();
    const auto PAYLOAD= KEY_PAYLOAD(info.keyID_);
    const uint16_t OPC= Layout_.I.opc= info.opcode_;

    if(OpcMap::SYSTEM == OPC) {
        Layout_.I.rd    = 0;
        Layout_.I.rs1   = 0;
        Layout_.I.fct3  = 0;
        Layout_.I.imm0tB= PAYLOAD;
        mnemonicHelper();
        return &Layout_;
    }

    if(ItypeImmShape::FENCE == DetectIImmShape(OPC, PAYLOAD)) {
        Layout_.I.rd  = 0;
        Layout_.I.rs1 = 0;
        Layout_.I.fct3= PAYLOAD; // 0=fence, 1=fence.i

        if(1 == Layout_.I.fct3) {
            Layout_.I.imm0tB= 0;
        } else if(InstAssembly_.size() >= 3) {
            const auto PRED= parseFenceSet(InstAssembly_.at(1));
            const auto SUCC= parseFenceSet(InstAssembly_.at(2));
            if(!PRED || !SUCC) {
                return util::Unexpected(
                    util::Error::InvalidArg("fence expects pred,succ (e.g. rw,rw or iorw,iorw)"));
            }
            Layout_.I.imm0tB= PACK_FENCE_IMM(0, *PRED, *SUCC);
        } else if(1 == InstAssembly_.size()) {
            Layout_.I.imm0tB= PACK_FENCE_IMM(0, G_FENCE_IORW, G_FENCE_IORW); // bare `fence` => iorw,iorw
        } else {
            return util::Unexpected(
                util::Error::InvalidArg("fence expects `fence` or `fence pred, succ`"));
        }

        mnemonicHelper();
        return &Layout_;
    }

    Layout_.I.fct3  = PAYLOAD;
    const auto SHAPE= DetectIImmShape(OPC, Layout_.I.fct3);
    Layout_.I.imm0tB= ItypeImmShape::SHIFT == SHAPE ? PAYLOAD >> 3 << 5 : 0;

    if(InstAssembly_.size() >= 4) {
        if(auto rd= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) Layout_.I.rd= *rd;
        if(auto rs1= isa::LOOKUP_REG_IDX(InstAssembly_.at(2))) Layout_.I.rs1= *rs1;

        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(3), imm)) {
            Layout_.I.imm0tB= ItypeImmShape::SHIFT == SHAPE
                                ? Layout_.I.imm0tB & 0xFE0U | imm & 0x1FU
                                : imm;
        }
    }

    mnemonicHelper();
    return &Layout_;
}

auto IType::calculateKeyID() -> KeyT
{
    switch(Layout_.I.opc) {
    case OpcMap::OP_IMM: {
        const uint8_t IMM7=
            ItypeImmShape::SHIFT == DetectIImmShape(Layout_.I.opc, Layout_.I.fct3) ? Layout_.I.imm0tB >> 5 : 0;
        Key_= OP_IMM_K(IMM7, Layout_.I.fct3, Layout_.I.opc);
    } break;
    case OpcMap::SYSTEM: Key_= SYSTEM_K(Layout_.I.imm0tB, Layout_.I.opc); break;
    default:             Key_= OP_K(Layout_.I.fct3, Layout_.I.opc); break;
    }

    return Key_;
}

auto IType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
