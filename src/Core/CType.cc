#include "Core/CType.hh"

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

constexpr uint16_t C_RAW(const InstLayout &l) noexcept { return static_cast<uint16_t>(l.entity_ & 0xFFFFU); }

[[nodiscard]] constexpr uint8_t C_QUAD(uint16_t c) noexcept { return c & 0x3U; }

[[nodiscard]] constexpr uint8_t C_F3(uint16_t c) noexcept { return (c >> 13) & 0x7U; }

[[nodiscard]] constexpr uint8_t C_RD(uint16_t c) noexcept { return (c >> 7) & 0x1FU; }

[[nodiscard]] constexpr uint8_t C_RS2(uint16_t c) noexcept { return (c >> 2) & 0x1FU; }

[[nodiscard]] constexpr uint8_t C_RD3(uint16_t c) noexcept { return (c >> 2) & 0x7U; }

[[nodiscard]] constexpr uint8_t C_RS1_3(uint16_t c) noexcept { return static_cast<uint8_t>((c >> 7) & 0x7U); }

void orU16(uint16_t &c, uint32_t bits) noexcept { c= static_cast<uint16_t>(c | bits); }

// Sign-extend imm[5:0]; cast is required so the arithmetic right-shift is signed.
[[nodiscard]] int32_t signExtend6(uint32_t imm6)
{
    const int32_t S= static_cast<int32_t>(imm6);
    return (S << 26) >> 26;
}

[[nodiscard]] int32_t decodeCiImm6(uint16_t c)
{
    const uint32_t IMM= (c >> 12 & 0x1U) << 5 | (c >> 2 & 0x1FU);
    return signExtend6(IMM);
}

void encodeCiImm6(uint16_t &c, int32_t imm)
{
    const uint32_t U= static_cast<uint32_t>(imm) & 0x3FU;
    c&= static_cast<uint16_t>(~((0x1FU << 2) | (0x1U << 12)));
    orU16(c, (U & 0x1FU) << 2);
    orU16(c, (U >> 5 & 0x1U) << 12);
}

[[nodiscard]] int32_t decodeCssUimm7(uint16_t c, uint32_t scale)
{
    return static_cast<int32_t>(((c >> 7) & 0x7FU) * scale);
}

void encodeCssUimm7(uint16_t &c, int32_t byteOff, uint32_t scale)
{
    const uint32_t UIMM= static_cast<uint32_t>(byteOff / static_cast<int32_t>(scale)) & 0x3FU;
    c&= static_cast<uint16_t>(~(0x3FU << 7)); // CSS uimm[5:0] in [12:7]; keep funct3[15:13]
    orU16(c, UIMM << 7);
}

[[nodiscard]] int32_t decodeCiSpLoadImm(uint16_t c)
{
    const uint32_t UIMM= (c >> 12 & 0x1U) << 5 | (c >> 2 & 0x7U) << 2 | (c >> 5 & 0x3U) << 6
                       | (c >> 4 & 0x4U) << 6;
    return static_cast<int32_t>(UIMM * 4U);
}

void encodeCiSpLoad(uint16_t &c, uint8_t rd, int32_t byteOff, uint8_t f3)
{
    c&= 0x3U;
    orU16(c, uint32_t { f3 } << 13);
    orU16(c, uint32_t { rd } << 7);
    const uint32_t UIMM= static_cast<uint32_t>(byteOff / 4) & 0x3FU;
    orU16(c, (UIMM >> 5 & 0x1U) << 12);
    orU16(c, (UIMM >> 2 & 0x7U) << 2);
    orU16(c, (UIMM >> 4 & 0x1U) << 4);
    orU16(c, (UIMM >> 6 & 0x3U) << 5);
}

// CL/CS uimm[7:2] (byte offset / scale); rd'/rs2' in [4:2], rs1' in [9:7].
[[nodiscard]] int32_t decodeClUimm(uint16_t c, uint32_t scale)
{
    const uint32_t UIMM= (c >> 10 & 0x7U) << 3 | (c >> 5 & 0x3U) << 6 | (c >> 2 & 0x7U) << 2
                       | (c >> 12 & 0x1U) << 5;
    return static_cast<int32_t>(UIMM * scale);
}

void encodeClCs(uint16_t &c, uint8_t reg3, uint8_t rs1p, uint32_t uimm, uint8_t f3)
{
    c&= 0x3U;
    orU16(c, uint32_t { f3 } << 13);
    orU16(c, (uimm >> 5 & 0x1U) << 12);
    orU16(c, (uimm >> 3 & 0x3U) << 10);
    orU16(c, (rs1p & 0x7U) << 7);
    orU16(c, (uimm >> 6 & 0x3U) << 5);
    orU16(c, (reg3 & 0x7U) << 2);
}

// CIW nzuimm[9:2] (byte offset, multiple of 4); rd' in [4:2].
[[nodiscard]] int32_t decodeCiwUimm(uint16_t c)
{
    const uint32_t NZ= (c >> 12 & 0x1U) << 5 | (c >> 7 & 0xFU) << 2 | (c >> 6 & 0x1U) << 6
                     | (c >> 5 & 0x1U) << 5 | (c >> 2 & 0x3U) | (c >> 11 & 0x3U) << 4;
    return static_cast<int32_t>(NZ * 4);
}

void encodeCiwUimm(uint16_t &c, int32_t byteOff)
{
    const uint32_t NZ= static_cast<uint32_t>(byteOff / 4) & 0xFFU;
    const uint8_t R3 = static_cast<uint8_t>((c >> 2) & 0x7U);
    c                = static_cast<uint16_t>(c & 0x3U);
    orU16(c, (NZ >> 6 & 0x1U) << 12);
    orU16(c, (NZ >> 4 & 0x3U) << 11);
    orU16(c, (NZ >> 2 & 0xFU) << 7);
    orU16(c, (NZ >> 6 & 0x1U) << 6);
    orU16(c, (NZ >> 5 & 0x1U) << 5);
    orU16(c, ((R3 & 0x7U) << 2) | (NZ & 0x3U));
}

[[nodiscard]] int32_t decodeCbImm9(uint16_t c)
{
    const auto IMM= static_cast<int32_t>((c >> 12 & 0x1) << 8 | (c >> 5 & 0x3U) << 6 | (c >> 2 & 0x7U) << 3
                                         | (c >> 10 & 0x3U) << 1 | (c >> 12 & 0x1));
    return IMM << 23 >> 23;
}

void encodeCbImm9(uint16_t &c, int32_t imm)
{
    const int32_t S= imm & 0x1FF;
    c&= static_cast<uint16_t>(~0x1F82U);
    orU16(c, (S >> 3 & 0x7U) << 2);
    orU16(c, (S >> 6 & 0x3U) << 5);
    orU16(c, (S >> 1 & 0x3U) << 10);
    orU16(c, (S >> 8 & 0x1U) << 12);
}

[[nodiscard]] int32_t decodeCjImm11(uint16_t c)
{
    const auto IMM= static_cast<int32_t>((c >> 12 & 0x1) << 10 | (c >> 2 & 0x1FU) << 5 | (c >> 7 & 0x1FU) << 1
                                         | (c >> 11 & 0x1) << 4 | (c >> 9 & 0x3U) << 8 | (c >> 8 & 0x1) << 3
                                         | (c >> 10 & 0x1) << 2);
    return IMM << 21 >> 21;
}

void encodeCjImm11(uint16_t &c, int32_t imm)
{
    const int32_t S= imm & 0x7FF;
    c&= static_cast<uint16_t>(~0x1FFEU);
    orU16(c, (S >> 1 & 0x1FU) << 7);
    orU16(c, (S >> 5 & 0x1FU) << 2);
    orU16(c, (S >> 10 & 0x1U) << 12);
    orU16(c, (S >> 11 & 0x1U) << 11);
    orU16(c, (S >> 9 & 0x1U) << 8);
    orU16(c, (S >> 7 & 0x3U) << 9);
    orU16(c, (S >> 4 & 0x1U) << 11);
}

[[nodiscard]] uint8_t calcSubFromInst(uint16_t c)
{
    const uint8_t Q = C_QUAD(c);
    const uint8_t F3= C_F3(c);

    if(0U == Q) {
        if(3U == F3) return 1U; // LD (same encoding as FLW)
        if(7U == F3) return 1U; // SD (same encoding as FSW)
        return 0U;
    }

    if(1U == Q) {
        if(0U == F3) {
            if(0U == C_RD(c) && 0 == decodeCiImm6(c)) return 1U; // C.NOP
            return 0U;
        }
        if(1U == F3) return 0U; // RV64C ADDIW over RV32C JAL
        if(3U == F3) {
            if(2U == C_RD(c)) return 1U; // C.ADDI16SP
            return 0U;                   // C.LUI
        }
        if(4U == F3) {
            const uint8_t F610= static_cast<uint8_t>((c >> 10) & 0x7U);
            const uint8_t F2  = static_cast<uint8_t>((c >> 5) & 0x3U);
            if(7U == F610) return static_cast<uint8_t>((0x7U << 2) | F2); // CA (sub/xor/or/and)
            if(3U == F610) return static_cast<uint8_t>((0x3U << 2) | F2); // CA (subw/addw)
            if(2U == F610) return 8U;                                     // C.ANDI
            if(0U != (c & 0x1000U)) return 1U;                            // C.SRAI
            return 0U;                                                    // C.SRLI
        }
        return 0U;
    }

    if(2U == Q) {
        if(3U == F3) return 1U; // LDSP
        if(7U == F3) return 1U; // SDSP
        if(4U == F3) {
            const uint8_t B12= static_cast<uint8_t>((c >> 12) & 0x1U);
            const uint8_t RS2= C_RS2(c);
            if(0U == B12) {
                if(0U == RS2) return 0U; // JR
                return 4U;               // MV
            }
            if(0U == RS2) {
                if(0U == C_RD(c)) return 2U; // EBREAK
                return 1U;                   // JALR
            }
            return 3U; // ADD
        }
        return 0U;
    }

    return 0U;
}

struct CPay {
    uint8_t f3_ {};
    uint8_t sub_ {};
    uint8_t q_ {};
};

[[nodiscard]] constexpr CPay UNPACK_PAY(CType::KeyT key) noexcept
{
    const CType::KeyT PAY= CType::KEY_PAYLOAD(key);
    return { .f3_ = static_cast<uint8_t>((PAY >> 8) & 0x7U),
             .sub_= static_cast<uint8_t>((PAY >> 2) & 0x3FU),
             .q_  = static_cast<uint8_t>(PAY & 0x3U) };
}

[[nodiscard]] bool instMatchesKey(uint16_t c, CType::KeyT key)
{
    const CPay EXP= UNPACK_PAY(key);
    if(C_QUAD(c) != EXP.q_ || C_F3(c) != EXP.f3_) return false;
    return calcSubFromInst(c) == EXP.sub_;
}

[[nodiscard]] CType::KeyT resolveDecodeKey(uint16_t c)
{
    for(const auto &entry: CType::G_INST_TABLE) {
        if(instMatchesKey(c, entry.keyID_)) return entry.keyID_;
    }
    return 0xffffU;
}

[[nodiscard]] std::string reg3Name(uint8_t r3, bool abi)
{
    return std::string(isa::LookupRegName(static_cast<uint8_t>(8U + r3), abi));
}

[[nodiscard]] std::string freg3Name(uint8_t r3, bool abi)
{
    return std::string(isa::LookupFregName(static_cast<uint8_t>(8U + r3), abi));
}

enum class CAsmKind : uint8_t {
    CIW_ADDI4SPN,
    CL_INT,
    CL_FP,
    CS_INT,
    CS_FP,
    CI,
    CI_LUI,
    CI_ADDI16SP,
    CB_SHIFT,
    CB_ANDI,
    CB_BRANCH,
    CA,
    CJ,
    CR,
    CSS_INT,
    CSS_FP,
    CI_SLLI,
    CI_SP_LOAD,
    CI_SP_FP_LOAD,
};

[[nodiscard]] CAsmKind kindFromKey(CType::KeyT key)
{
    const CPay P= UNPACK_PAY(key);

    if(0U == P.q_) {
        if(0U == P.f3_) return CAsmKind::CIW_ADDI4SPN;
        if(1U == P.f3_) return CAsmKind::CL_FP;
        if(5U == P.f3_) return CAsmKind::CS_FP;
        if(2U == P.f3_) return CAsmKind::CL_INT;
        if(6U == P.f3_) return CAsmKind::CS_INT;
        if(3U == P.f3_) return 0U == P.sub_ ? CAsmKind::CL_FP : CAsmKind::CL_INT;
        if(7U == P.f3_) return 0U == P.sub_ ? CAsmKind::CS_FP : CAsmKind::CS_INT;
    }
    if(1U == P.q_) {
        if(5U == P.f3_) return CAsmKind::CJ;
        if(6U == P.f3_ || 7U == P.f3_) return CAsmKind::CB_BRANCH;
        if(4U == P.f3_ && 8U == P.sub_) return CAsmKind::CB_ANDI;
        if(4U == P.f3_ && P.sub_ <= 1U) return CAsmKind::CB_SHIFT;
        if(4U == P.f3_) return CAsmKind::CA;
        if(3U == P.f3_) return 1U == P.sub_ ? CAsmKind::CI_ADDI16SP : CAsmKind::CI_LUI;
        return CAsmKind::CI;
    }
    if(2U == P.q_) {
        if(0U == P.f3_) return CAsmKind::CI_SLLI;
        if(1U == P.f3_) return CAsmKind::CI_SP_FP_LOAD;
        if(5U == P.f3_) return CAsmKind::CSS_FP;
        if(2U == P.f3_) return CAsmKind::CI_SP_LOAD;
        if(6U == P.f3_) return CAsmKind::CSS_INT;
        if(3U == P.f3_) return 0U == P.sub_ ? CAsmKind::CI_SP_FP_LOAD : CAsmKind::CI_SP_LOAD;
        if(4U == P.f3_) return CAsmKind::CR;
        if(7U == P.f3_) return 0U == P.sub_ ? CAsmKind::CSS_FP : CAsmKind::CSS_INT;
    }
    return CAsmKind::CI;
}

} // namespace

CType::CType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
    if(InstAssembly_.empty()) {
        Key_             = resolveDecodeKey(C_RAW(Layout_));
        NameAndXlenCache_= InstTable_->Find(Key_);
    }
}

CType::CType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(format, std::move(instAssembly), hasSetABI)
{
    init();
}

void CType::Parse()
{
    const uint16_t C= C_RAW(Layout_);
    setBitsField({ C & 0x3U, (C >> 2) & 0x3FFFU });
}

void CType::mnemonicHelper()
{
    const uint16_t C         = C_RAW(Layout_);
    const std::string_view NM= InstAssembly_.empty() ? LookupNameAndInfo().name_ : InstAssembly_.front();
    const CAsmKind KIND      = kindFromKey(Key_);

    if("c.nop" == NM || "c.ebreak" == NM) {
        if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
        return;
    }

    switch(KIND) {
    case CAsmKind::CIW_ADDI4SPN: {
        const auto RD= reg3Name(C_RD3(C), HasSetABI_);
        appendOperands({ " ", RD, ",", std::to_string(decodeCiwUimm(C)) });
    } break;
    case CAsmKind::CL_INT: {
        const auto RD       = reg3Name(C_RD3(C), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(C), HasSetABI_);
        const uint32_t SCALE= "c.ld" == NM ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(C, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CL_FP: {
        const auto RD       = freg3Name(C_RD3(C), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(C), HasSetABI_);
        const uint32_t SCALE= ("c.fld" == NM || "c.fldsp" == NM) ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(C, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_INT: {
        const auto RS2      = reg3Name(C_RD3(C), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(C), HasSetABI_);
        const uint32_t SCALE= "c.sd" == NM ? 8U : 4U;
        appendOperands({ " ", RS2, ",", std::to_string(decodeClUimm(C, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_FP: {
        const auto RS2      = freg3Name(C_RD3(C), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(C), HasSetABI_);
        const uint32_t SCALE= ("c.fsd" == NM || "c.fsdsp" == NM) ? 8U : 4U;
        appendOperands({ " ", RS2, ",", std::to_string(decodeClUimm(C, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CI: {
        if("c.jal" == NM) {
            appendOperands({ " ", std::to_string(decodeCiImm6(C)) });
        } else {
            appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_), ",", std::to_string(decodeCiImm6(C)) });
        }
    } break;
    case CAsmKind::CI_LUI:
        appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_), ",", std::to_string(decodeCiImm6(C)) });
        break;
    case CAsmKind::CI_ADDI16SP:
        appendOperands({ " ", std::to_string(decodeCiImm6(C) * 16) });
        break;
    case CAsmKind::CB_SHIFT: {
        const uint32_t SH= (C >> 2 & 0x1FU) | (C >> 12 & 0x1U) << 5;
        appendOperands({ " ", reg3Name(C_RD3(C), HasSetABI_), ",", std::to_string(SH) });
    } break;
    case CAsmKind::CB_ANDI:
        appendOperands({ " ", reg3Name(C_RD3(C), HasSetABI_), ",", std::to_string(decodeCiImm6(C)) });
        break;
    case CAsmKind::CB_BRANCH:
        appendOperands({ " ", reg3Name(C_RD3(C), HasSetABI_), ",", std::to_string(decodeCbImm9(C)) });
        break;
    case CAsmKind::CA:
        appendOperands({ " ", reg3Name(C_RS1_3(C), HasSetABI_), ",", reg3Name(C_RD3(C), HasSetABI_) });
        break;
    case CAsmKind::CJ:
        appendOperands({ " ", std::to_string(decodeCjImm11(C)) });
        break;
    case CAsmKind::CR: {
        if("c.jr" == NM || "c.jalr" == NM) {
            appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_) });
        } else if("c.mv" == NM || "c.add" == NM) {
            appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_), ",", isa::LookupRegName(C_RS2(C), HasSetABI_) });
        }
    } break;
    case CAsmKind::CSS_INT: {
        const uint32_t SCALE= "c.sdsp" == NM ? 8U : 4U;
        appendOperands({ " ", isa::LookupRegName(C_RS2(C), HasSetABI_), ",", std::to_string(decodeCssUimm7(C, SCALE)) });
    } break;
    case CAsmKind::CSS_FP: {
        const uint32_t SCALE= "c.fsdsp" == NM ? 8U : 4U;
        appendOperands({ " ", isa::LookupFregName(C_RS2(C), HasSetABI_), ",", std::to_string(decodeCssUimm7(C, SCALE)) });
    } break;
    case CAsmKind::CI_SLLI: {
        const uint32_t SH= (C >> 2 & 0x1FU) | (C >> 12 & 0x1U) << 5;
        appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_), ",", std::to_string(SH) });
    } break;
    case CAsmKind::CI_SP_LOAD:
        appendOperands({ " ", isa::LookupRegName(C_RD(C), HasSetABI_), ",", std::to_string(decodeCiSpLoadImm(C)) });
        break;
    case CAsmKind::CI_SP_FP_LOAD:
        appendOperands({ " ", isa::LookupFregName(C_RD(C), HasSetABI_), ",", std::to_string(decodeCiSpLoadImm(C)) });
        break;
    default: break;
    }
}

util::Expected_u<const InstLayout *> CType::Assembly()
{
    const auto &info= LookupIdxAndInfo();
    Key_            = info.keyID_;
    const CPay PAY  = UNPACK_PAY(info.keyID_);

    uint16_t c= PAY.q_;
    orU16(c, uint32_t { PAY.f3_ } << 13);

    const auto &mn     = InstAssembly_.at(0);
    const CAsmKind KIND= kindFromKey(info.keyID_);

    switch(KIND) {
    case CAsmKind::CIW_ADDI4SPN: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd3= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rd3 >= 8U) orU16(c, (*rd3 - 8U) << 2);
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiwUimm(c, imm);
        }
    } break;
    case CAsmKind::CL_INT:
    case CAsmKind::CL_FP:  {
        if(InstAssembly_.size() >= 4) {
            auto parseReg3= [&](std::string_view tok, bool fp) -> std::optional<uint8_t> {
                if(fp) {
                    if(auto i= isa::LookupFregIdx(tok)) return static_cast<uint8_t>(*i - 8U);
                } else if(auto i= isa::LookupRegIdx(tok)) {
                    return *i >= 8U ? std::make_optional(static_cast<uint8_t>(*i - 8U)) : std::nullopt;
                }
                return std::nullopt;
            };
            const bool FP= CAsmKind::CL_FP == KIND;
            uint8_t rdp {};
            uint8_t rs1p {};
            if(auto rd= parseReg3(InstAssembly_.at(1), FP)) rdp= *rd;
            if(auto rs1= parseReg3(InstAssembly_.at(3), false)) rs1p= *rs1;
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) {
                const uint32_t SCALE= ("c.ld" == mn || "c.fld" == mn) ? 8U : 4U;
                const uint32_t UIMM = static_cast<uint32_t>(imm / static_cast<int32_t>(SCALE));
                encodeClCs(c, rdp, rs1p, UIMM, PAY.f3_);
            }
        }
    } break;
    case CAsmKind::CS_INT:
    case CAsmKind::CS_FP:  {
        if(InstAssembly_.size() >= 4) {
            const bool FP= CAsmKind::CS_FP == KIND;
            uint8_t rs2p {};
            uint8_t rs1p {};
            if(FP) {
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(1))) rs2p= static_cast<uint8_t>(*rs2 - 8U);
            } else if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rs2 >= 8U) rs2p= static_cast<uint8_t>(*rs2 - 8U);
            }
            if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(3))) {
                if(*rs1 >= 8U) rs1p= static_cast<uint8_t>(*rs1 - 8U);
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) {
                const uint32_t SCALE= ("c.sd" == mn || "c.fsd" == mn) ? 8U : 4U;
                const uint32_t UIMM = static_cast<uint32_t>(imm / static_cast<int32_t>(SCALE));
                encodeClCs(c, rs2p, rs1p, UIMM, PAY.f3_);
            }
        }
    } break;
    case CAsmKind::CI: {
        if("c.jal" == mn) {
            orU16(c, 0x1U << 7);
            if(InstAssembly_.size() >= 2) {
                int32_t imm {};
                if(util::ParseInt(InstAssembly_.at(1), imm)) encodeCiImm6(c, imm);
            }
        } else if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiImm6(c, imm);
        }
    } break;
    case CAsmKind::CI_LUI: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiImm6(c, imm);
        }
    } break;
    case CAsmKind::CI_ADDI16SP: {
        orU16(c, 0x2U << 7);
        if(InstAssembly_.size() >= 2) {
            int32_t imm {};
            const std::string_view IMM_TOK= InstAssembly_.size() >= 3 ? InstAssembly_.at(2)
                                                                      : InstAssembly_.at(1);
            if(util::ParseInt(IMM_TOK, imm)) encodeCiImm6(c, imm / 16);
        }
    } break;
    case CAsmKind::CB_SHIFT: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 2);
            }
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) {
                orU16(c, (sh & 0x1FU) << 2);
                orU16(c, (sh >> 5 & 0x1U) << 12);
            }
            if("c.srai" == mn) orU16(c, 0x1U << 12);
        }
    } break;
    case CAsmKind::CB_ANDI: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 2);
            }
            orU16(c, 0x2U << 10); // funct3=100, [12:10]=010 for C.ANDI
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiImm6(c, imm);
        }
    } break;
    case CAsmKind::CB_BRANCH: {
        if(InstAssembly_.size() >= 3) {
            if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rs1 >= 8U) orU16(c, (*rs1 - 8U) << 2);
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCbImm9(c, imm);
        }
    } break;
    case CAsmKind::CA: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 7);
            }
            if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(2))) {
                if(*rs2 >= 8U) orU16(c, (*rs2 - 8U) << 2);
            }
            orU16(c, (uint32_t { PAY.sub_ } >> 2) << 10);
            orU16(c, (uint32_t { PAY.sub_ } & 0x3U) << 5);
        }
    } break;
    case CAsmKind::CJ: {
        if(InstAssembly_.size() >= 2) {
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(1), imm)) encodeCjImm11(c, imm);
        }
    } break;
    case CAsmKind::CR: {
        if("c.ebreak" == mn) {
            orU16(c, 0x1U << 12);
        } else if("c.jr" == mn || "c.jalr" == mn) {
            if(InstAssembly_.size() >= 2) {
                if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rs1 } << 7);
            }
            if("c.jalr" == mn) orU16(c, 0x1U << 12);
        } else if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(2))) orU16(c, uint32_t { *rs2 } << 2);
            if("c.add" == mn) orU16(c, 0x1U << 12);
        }
    } break;
    case CAsmKind::CSS_INT:
    case CAsmKind::CSS_FP:  {
        if(InstAssembly_.size() >= 3) {
            const bool FP= CAsmKind::CSS_FP == KIND;
            if(FP) {
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rs2 } << 2);
            } else if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(1))) {
                orU16(c, uint32_t { *rs2 } << 2);
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) {
                const uint32_t SCALE= ("c.sdsp" == mn || "c.fsdsp" == mn) ? 8U : 4U;
                encodeCssUimm7(c, imm, SCALE);
            }
        }
    } break;
    case CAsmKind::CI_SLLI: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) {
                orU16(c, (sh & 0x1FU) << 2);
                orU16(c, (sh >> 5 & 0x1U) << 12);
            }
        }
    } break;
    case CAsmKind::CI_SP_LOAD:
    case CAsmKind::CI_SP_FP_LOAD: {
        if(InstAssembly_.size() >= 3) {
            uint8_t rd {};
            if(CAsmKind::CI_SP_FP_LOAD == KIND) {
                if(auto r= isa::LookupFregIdx(InstAssembly_.at(1))) rd= *r;
            } else if(auto r= isa::LookupRegIdx(InstAssembly_.at(1))) {
                rd= *r;
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiSpLoad(c, rd, imm, PAY.f3_);
        }
    } break;
    default: break;
    }

    Layout_.entity_= c;
    mnemonicHelper();
    return &Layout_;
}

auto CType::calculateKeyID() -> KeyT
{
    Key_= resolveDecodeKey(C_RAW(Layout_));
    return Key_;
}

auto CType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
