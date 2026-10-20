#include "Core/IType.hh"
#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

constexpr uint8_t G_FENCE_IORW    = 0xFU;   // I|O|R|W
constexpr uint16_t G_FENCE_TSO_IMM= 0x833U; // fm=1000, pred=0011, succ=0011
constexpr uint16_t G_PAUSE_IMM    = 0x010U; // fm=0000, pred=0001, succ=0000

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

[[nodiscard]] constexpr bool IS_SPECIAL_FENCE_IMM(uint32_t imm) noexcept
{
    return G_FENCE_TSO_IMM == imm || G_PAUSE_IMM == imm;
}

// Sign-extend imm[11:0]; cast is required so the arithmetic right-shift is signed.
[[nodiscard]] int32_t decodeIImm12(uint32_t imm)
{
    const int32_t S_IMM= static_cast<int32_t>(imm);
    return (S_IMM << 20) >> 20;
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
    case ItypeImmShape::SHIFT: {
        // OP_IMM: up to 7-bit shamt (RV128); OP_IMM_64: 6-bit; OP_IMM_32: 5-bit
        if(OpcMap::OP_IMM == Layout_.I.opc) {
            setBitsField({ Layout_.I.opc, Layout_.I.rd, Layout_.I.fct3, Layout_.I.rs1, IMM & 0x7FU, IMM >> 7 });
        } else if(OpcMap::OP_IMM_64 == Layout_.I.opc) {
            setBitsField({ Layout_.I.opc, Layout_.I.rd, Layout_.I.fct3, Layout_.I.rs1, IMM & 0x3FU, IMM >> 6 });
        } else {
            setBitsField({ Layout_.I.opc, Layout_.I.rd, Layout_.I.fct3, Layout_.I.rs1, IMM & 0x1FU, IMM >> 5 });
        }
    } break;
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
        if(1 == Layout_.I.fct3 || IS_SPECIAL_FENCE_IMM(Layout_.I.imm0tB)) {
            if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
            return;
        }
        const auto PRED= formatFenceSet(Layout_.I.imm0tB >> 4 & 0xFU);
        const auto SUCC= formatFenceSet(Layout_.I.imm0tB & 0xFU);
        appendOperands({ " ", PRED, ",", SUCC });
        return;
    }

    auto rd = isa::LookupRegName(Layout_.I.rd, HasSetABI_);
    auto rs1= isa::LookupRegName(Layout_.I.rs1, HasSetABI_);
    std::string immStr;
    if(ItypeImmShape::SHIFT == SHAPE) {
        uint32_t shamtMask= 0x1FU;
        if(OpcMap::OP_IMM == OPC) shamtMask= 0x7FU;
        else if(OpcMap::OP_IMM_64 == OPC) shamtMask= 0x3FU;
        immStr= std::to_string(Layout_.I.imm0tB & shamtMask);
    } else {
        immStr= std::to_string(decodeIImm12(Layout_.I.imm0tB));
    }
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

    if(OpcMap::MISC_MEM == OPC) {
        if(IS_SPECIAL_FENCE_IMM(PAYLOAD)) {
            Layout_.I.rd    = 0;
            Layout_.I.rs1   = 0;
            Layout_.I.fct3  = 0;
            Layout_.I.imm0tB= PAYLOAD;
            mnemonicHelper();
            return &Layout_;
        }

        // f3=2 â†?RV128I lq (I-imm load); f3=0/1 â†?fence*
        if(PAYLOAD <= 1) {
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
                Layout_.I.imm0tB= PACK_FENCE_IMM(0, G_FENCE_IORW, G_FENCE_IORW);
            } else {
                return util::Unexpected(
                    util::Error::InvalidArg("fence expects `fence` or `fence pred, succ`"));
            }
            mnemonicHelper();
            return &Layout_;
        }
        // fall through: lq etc.
    }

    if(OpcMap::OP_IMM == OPC || OpcMap::OP_IMM_32 == OPC || OpcMap::OP_IMM_64 == OPC) {
        Layout_.I.fct3  = PAYLOAD;
        const auto SHAPE= DetectIImmShape(OPC, Layout_.I.fct3);
        Layout_.I.imm0tB= ItypeImmShape::SHIFT == SHAPE ? PAYLOAD >> 3 << 5 : 0;

        if(InstAssembly_.size() >= 4) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) Layout_.I.rd= *rd;
            if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(2))) Layout_.I.rs1= *rs1;

            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(3), imm)) {
                if(ItypeImmShape::SHIFT == SHAPE) {
                    uint32_t shamtMask= 0x1FU;
                    uint32_t functMask= 0xFE0U;
                    if(OpcMap::OP_IMM == OPC) {
                        shamtMask= 0x7FU;
                        functMask= 0xF80U;
                    } else if(OpcMap::OP_IMM_64 == OPC) {
                        shamtMask= 0x3FU;
                        functMask= 0xFC0U;
                    }
                    Layout_.I.imm0tB= Layout_.I.imm0tB & functMask | imm & shamtMask;
                } else {
                    Layout_.I.imm0tB= imm & 0xFFFU;
                }
            }
        }
        mnemonicHelper();
        return &Layout_;
    }

    // LOAD / JALR / LQ (MISC_MEM f3=2)
    Layout_.I.fct3  = PAYLOAD;
    Layout_.I.imm0tB= 0;

    if(InstAssembly_.size() >= 4) {
        if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) Layout_.I.rd= *rd;
        if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(2))) Layout_.I.rs1= *rs1;

        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(3), imm)) {
            Layout_.I.imm0tB= imm & 0xFFFU;
        }
    }

    mnemonicHelper();
    return &Layout_;
}

auto IType::calculateKeyID() -> KeyT
{
    switch(Layout_.I.opc) {
    case OpcMap::OP_IMM:
    case OpcMap::OP_IMM_32:
    case OpcMap::OP_IMM_64: {
        // imm[10] only â€?wider shamts must not disturb the key
        const uint8_t FUNCT=
            ItypeImmShape::SHIFT == DetectIImmShape(Layout_.I.opc, Layout_.I.fct3)
                ? (Layout_.I.imm0tB >> 10 & 1 ? 0x20 : 0)
                : 0;
        Key_= OP_IMM_K(FUNCT, Layout_.I.fct3, Layout_.I.opc);
    } break;
    case OpcMap::MISC_MEM:
        if(0 == Layout_.I.fct3 && IS_SPECIAL_FENCE_IMM(Layout_.I.imm0tB)) {
            Key_= PACK_KEY(Layout_.I.imm0tB, OpcMap::MISC_MEM);
        } else {
            Key_= OP_K(Layout_.I.fct3, OpcMap::MISC_MEM);
        }
        break;
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
