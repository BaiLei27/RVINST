#include "Core/CType.hh"

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

// Pack Layout_.C (op|b2) into the 16-bit RVC word used by scrambled-imm helpers.
[[nodiscard]] constexpr uint16_t C_RAW(const InstLayout &l) noexcept
{
    return static_cast<uint16_t>((l.C.b2 << 2) | l.C.opc);
}

// Write a 16-bit RVC word through Layout_.C; clears entity_[31:16].
void setCRaw(InstLayout &l, uint16_t c) noexcept
{
    l.entity_= 0;
    l.C.opc  = c & 0x3U;
    l.C.b2   = (c >> 2) & 0x3FFFU;
}

[[nodiscard]] constexpr uint8_t C_QUAD(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.C.opc); }

[[nodiscard]] constexpr uint8_t C_F3(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.CI.fct3); }

[[nodiscard]] constexpr uint8_t C_RD(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.CI.rd); }

[[nodiscard]] constexpr uint8_t C_RS2(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.CR.rs2); }

[[nodiscard]] constexpr uint8_t C_RD3(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.CL.rd3); }

[[nodiscard]] constexpr uint8_t C_RS1_3(const InstLayout &l) noexcept { return static_cast<uint8_t>(l.CL.rs13); }

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

[[nodiscard]] int32_t decodeCssUimm7(uint16_t c, unsigned scale)
{
    return static_cast<int32_t>(((c >> 7) & 0x7FU) * scale);
}

void encodeCssUimm7(uint16_t &c, int32_t byteOff, unsigned scale)
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
[[nodiscard]] int32_t decodeClUimm(uint16_t c, unsigned scale)
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
    const int32_t IMM= (c >> 12 & 0x1) << 8 | (c >> 5 & 0x3U) << 6 | (c >> 2 & 0x7U) << 3
                     | (c >> 10 & 0x3U) << 1 | (c >> 12 & 0x1);
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
    const int32_t IMM= (c >> 12 & 0x1) << 10 | (c >> 2 & 0x1FU) << 5 | (c >> 7 & 0x1FU) << 1
                     | (c >> 11 & 0x1) << 4 | (c >> 9 & 0x3U) << 8 | (c >> 8 & 0x1) << 3
                     | (c >> 10 & 0x1) << 2;
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

[[nodiscard]] uint8_t calcSubFromInst(const InstLayout &l)
{
    const uint8_t Q = C_QUAD(l);
    const uint8_t F3= C_F3(l);

    if(0U == Q) {
        if(3U == F3) return 1U; // LD (same encoding as FLW)
        if(7U == F3) return 1U; // SD (same encoding as FSW)
        return 0U;
    }

    if(1U == Q) {
        if(0U == F3) {
            if(0U == C_RD(l) && 0 == decodeCiImm6(C_RAW(l))) return 1U; // C.NOP
            return 0U;
        }
        if(1U == F3) return 0U; // RV64C ADDIW over RV32C JAL
        if(3U == F3) {
            if(2U == C_RD(l)) return 1U; // C.ADDI16SP
            return 0U;                   // C.LUI
        }
        if(4U == F3) {
            const uint8_t F610= static_cast<uint8_t>(l.CA.fct6 & 0x7U); // inst[12:10]
            const uint8_t F2  = static_cast<uint8_t>(l.CA.fct2);
            if(7U == F610) return static_cast<uint8_t>((0x7U << 2) | F2); // CA (sub/xor/or/and)
            if(3U == F610) return static_cast<uint8_t>((0x3U << 2) | F2); // CA (subw/addw)
            if(2U == F610) return 8U;                                     // C.ANDI
            if(0U != l.CI.imm5) return 1U;                                // C.SRAI
            return 0U;                                                    // C.SRLI
        }
        return 0U;
    }

    if(2U == Q) {
        if(3U == F3) return 1U; // LDSP
        if(7U == F3) return 1U; // SDSP
        if(4U == F3) {
            const uint8_t B12= static_cast<uint8_t>(l.CI.imm5);
            const uint8_t RS2= C_RS2(l);
            if(0U == B12) {
                if(0U == RS2) return 0U; // JR
                return 4U;               // MV
            }
            if(0U == RS2) {
                if(0U == C_RD(l)) return 2U; // EBREAK
                return 1U;                   // JALR
            }
            return 3U; // ADD
        }
        return 0U;
    }

    return 0U;
}

struct CPay {
    uint8_t f3 {};
    uint8_t sub {};
    uint8_t q {};
};

[[nodiscard]] constexpr CPay unpackPay(CType::KeyT key) noexcept
{
    const CType::KeyT PAY= CType::KEY_PAYLOAD(key);
    return { .f3 = static_cast<uint8_t>((PAY >> 8) & 0x7U),
             .sub= static_cast<uint8_t>((PAY >> 2) & 0x3FU),
             .q  = static_cast<uint8_t>(PAY & 0x3U) };
}

[[nodiscard]] bool instMatchesKey(const InstLayout &l, CType::KeyT key)
{
    const CPay EXP= unpackPay(key);
    if(C_QUAD(l) != EXP.q || C_F3(l) != EXP.f3) return false;
    return calcSubFromInst(l) == EXP.sub;
}

[[nodiscard]] CType::KeyT resolveDecodeKey(const InstLayout &l)
{
    for(const auto &entry: CType::G_INST_TABLE) {
        if(instMatchesKey(l, entry.keyID_)) return entry.keyID_;
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

[[nodiscard]] CAsmKind kindFromKey(CType::KeyT key)
{
    const CPay P= unpackPay(key);

    if(0U == P.q) {
        if(0U == P.f3) return CAsmKind::CIW_ADDI4SPN;
        if(1U == P.f3) return CAsmKind::CL_FP;
        if(5U == P.f3) return CAsmKind::CS_FP;
        if(2U == P.f3) return CAsmKind::CL_INT;
        if(6U == P.f3) return CAsmKind::CS_INT;
        if(3U == P.f3) return 0U == P.sub ? CAsmKind::CL_FP : CAsmKind::CL_INT;
        if(7U == P.f3) return 0U == P.sub ? CAsmKind::CS_FP : CAsmKind::CS_INT;
    }
    if(1U == P.q) {
        if(5U == P.f3) return CAsmKind::CJ;
        if(6U == P.f3 || 7U == P.f3) return CAsmKind::CB_BRANCH;
        if(4U == P.f3 && 8U == P.sub) return CAsmKind::CB_ANDI;
        if(4U == P.f3 && P.sub <= 1U) return CAsmKind::CB_SHIFT;
        if(4U == P.f3) return CAsmKind::CA;
        if(3U == P.f3) return 1U == P.sub ? CAsmKind::CI_ADDI16SP : CAsmKind::CI_LUI;
        return CAsmKind::CI;
    }
    if(2U == P.q) {
        if(0U == P.f3) return CAsmKind::CI_SLLI;
        if(1U == P.f3) return CAsmKind::CI_SP_FP_LOAD;
        if(5U == P.f3) return CAsmKind::CSS_FP;
        if(2U == P.f3) return CAsmKind::CI_SP_LOAD;
        if(6U == P.f3) return CAsmKind::CSS_INT;
        if(3U == P.f3) return 0U == P.sub ? CAsmKind::CI_SP_FP_LOAD : CAsmKind::CI_SP_LOAD;
        if(4U == P.f3) return CAsmKind::CR;
        if(7U == P.f3) return 0U == P.sub ? CAsmKind::CSS_FP : CAsmKind::CSS_INT;
    }
    return CAsmKind::CI;
}

} // namespace

CType::CType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
    if(InstAssembly_.empty()) {
        Key_             = resolveDecodeKey(Layout_);
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
    switch(DetectCShape(Layout_.entity_)) {
    case CShape::CR:
        setBitsField({ Layout_.CR.opc, Layout_.CR.rs2, Layout_.CR.rd, Layout_.CR.fct4 });
        break;
    case CShape::CI:
        setBitsField({ Layout_.CI.opc, Layout_.CI.imm0t4, Layout_.CI.rd, Layout_.CI.imm5, Layout_.CI.fct3 });
        break;
    case CShape::CSS:
        setBitsField({ Layout_.CSS.opc, Layout_.CSS.rs2, Layout_.CSS.imm6, Layout_.CSS.fct3 });
        break;
    case CShape::CIW:
        setBitsField({ Layout_.CIW.opc, Layout_.CIW.rd3, Layout_.CIW.imm8, Layout_.CIW.fct3 });
        break;
    case CShape::CL:
        setBitsField({ Layout_.CL.opc,
                       Layout_.CL.rd3,
                       Layout_.CL.immLo,
                       Layout_.CL.rs13,
                       Layout_.CL.immHi,
                       Layout_.CL.fct3 });
        break;
    case CShape::CS:
        setBitsField({ Layout_.CS.opc,
                       Layout_.CS.rs23,
                       Layout_.CS.immLo,
                       Layout_.CS.rs13,
                       Layout_.CS.immHi,
                       Layout_.CS.fct3 });
        break;
    case CShape::CA:
        setBitsField({ Layout_.CA.opc, Layout_.CA.rs23, Layout_.CA.fct2, Layout_.CA.rd3, Layout_.CA.fct6 });
        break;
    case CShape::CB:
        setBitsField({ Layout_.CB.opc, Layout_.CB.offLo, Layout_.CB.rd3, Layout_.CB.offHi, Layout_.CB.fct3 });
        break;
    case CShape::CJ:
        setBitsField({ Layout_.CJ.opc, Layout_.CJ.target, Layout_.CJ.fct3 });
        break;
    }
}

void CType::mnemonicHelper()
{
    const uint16_t RAW       = C_RAW(Layout_);
    const std::string_view NM= InstAssembly_.empty() ? LookupNameAndInfo().name_ : InstAssembly_.front();
    const CAsmKind KIND      = kindFromKey(Key_);

    if("c.nop" == NM || "c.ebreak" == NM) {
        if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
        return;
    }

    switch(KIND) {
    case CAsmKind::CIW_ADDI4SPN: {
        const auto RD= reg3Name(C_RD3(Layout_), HasSetABI_);
        appendOperands({ " ", RD, ",", std::to_string(decodeCiwUimm(RAW)) });
    } break;
    case CAsmKind::CL_INT: {
        const auto RD       = reg3Name(C_RD3(Layout_), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const unsigned SCALE= "c.ld" == NM ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CL_FP: {
        const auto RD       = freg3Name(C_RD3(Layout_), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const unsigned SCALE= ("c.fld" == NM || "c.fldsp" == NM) ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_INT: {
        const auto RS2      = reg3Name(static_cast<uint8_t>(Layout_.CS.rs23), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const unsigned SCALE= "c.sd" == NM ? 8U : 4U;
        appendOperands({ " ", RS2, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_FP: {
        const auto RS2      = freg3Name(static_cast<uint8_t>(Layout_.CS.rs23), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const unsigned SCALE= ("c.fsd" == NM || "c.fsdsp" == NM) ? 8U : 4U;
        appendOperands({ " ", RS2, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CI: {
        if("c.jal" == NM) {
            appendOperands({ " ", std::to_string(decodeCiImm6(RAW)) });
        } else {
            appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) });
        }
    } break;
    case CAsmKind::CI_LUI:
        appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) });
        break;
    case CAsmKind::CI_ADDI16SP:
        appendOperands({ " ", std::to_string(decodeCiImm6(RAW) * 16) });
        break;
    case CAsmKind::CB_SHIFT: {
        const uint32_t SH= Layout_.CI.imm0t4 | Layout_.CI.imm5 << 5;
        appendOperands({ " ", reg3Name(static_cast<uint8_t>(Layout_.CB.rd3), HasSetABI_), ",", std::to_string(SH) });
    } break;
    case CAsmKind::CB_ANDI:
        appendOperands({ " ", reg3Name(static_cast<uint8_t>(Layout_.CB.rd3), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) });
        break;
    case CAsmKind::CB_BRANCH:
        appendOperands({ " ", reg3Name(static_cast<uint8_t>(Layout_.CB.rd3), HasSetABI_), ",", std::to_string(decodeCbImm9(RAW)) });
        break;
    case CAsmKind::CA:
        appendOperands({ " ", reg3Name(static_cast<uint8_t>(Layout_.CA.rd3), HasSetABI_), ",", reg3Name(static_cast<uint8_t>(Layout_.CA.rs23), HasSetABI_) });
        break;
    case CAsmKind::CJ:
        appendOperands({ " ", std::to_string(decodeCjImm11(RAW)) });
        break;
    case CAsmKind::CR: {
        if("c.jr" == NM || "c.jalr" == NM) {
            appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_) });
        } else if("c.mv" == NM || "c.add" == NM) {
            appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", isa::LookupRegName(C_RS2(Layout_), HasSetABI_) });
        }
    } break;
    case CAsmKind::CSS_INT: {
        const unsigned SCALE= "c.sdsp" == NM ? 8U : 4U;
        appendOperands({ " ", isa::LookupRegName(C_RS2(Layout_), HasSetABI_), ",", std::to_string(decodeCssUimm7(RAW, SCALE)) });
    } break;
    case CAsmKind::CSS_FP: {
        const unsigned SCALE= "c.fsdsp" == NM ? 8U : 4U;
        appendOperands({ " ", isa::LookupFregName(C_RS2(Layout_), HasSetABI_), ",", std::to_string(decodeCssUimm7(RAW, SCALE)) });
    } break;
    case CAsmKind::CI_SLLI: {
        const uint32_t SH= Layout_.CI.imm0t4 | Layout_.CI.imm5 << 5;
        appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(SH) });
    } break;
    case CAsmKind::CI_SP_LOAD:
        appendOperands({ " ", isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(decodeCiSpLoadImm(RAW)) });
        break;
    case CAsmKind::CI_SP_FP_LOAD:
        appendOperands({ " ", isa::LookupFregName(C_RD(Layout_), HasSetABI_), ",", std::to_string(decodeCiSpLoadImm(RAW)) });
        break;
    default: break;
    }
}

util::Expected_u<const InstLayout *> CType::Assembly()
{
    const auto &info= LookupIdxAndInfo();
    Key_            = info.keyID_;
    const CPay PAY  = unpackPay(info.keyID_);

    uint16_t c= PAY.q;
    orU16(c, uint32_t { PAY.f3 } << 13);

    const auto &MN     = InstAssembly_.at(0);
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
                const unsigned SCALE= ("c.ld" == MN || "c.fld" == MN) ? 8U : 4U;
                const uint32_t UIMM = static_cast<uint32_t>(imm / static_cast<int32_t>(SCALE));
                encodeClCs(c, rdp, rs1p, UIMM, PAY.f3);
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
                const unsigned SCALE= ("c.sd" == MN || "c.fsd" == MN) ? 8U : 4U;
                const uint32_t UIMM = static_cast<uint32_t>(imm / static_cast<int32_t>(SCALE));
                encodeClCs(c, rs2p, rs1p, UIMM, PAY.f3);
            }
        }
    } break;
    case CAsmKind::CI: {
        if("c.jal" == MN) {
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
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 7); // CB rd' in [9:7]
            }
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) {
                orU16(c, (sh & 0x1FU) << 2);
                orU16(c, (sh >> 5 & 0x1U) << 12);
            }
            if("c.srai" == MN) orU16(c, 0x1U << 12);
        }
    } break;
    case CAsmKind::CB_ANDI: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 7); // CB rd' in [9:7]
            }
            orU16(c, 0x2U << 10); // funct3=100, [12:10]=010 for C.ANDI
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiImm6(c, imm);
        }
    } break;
    case CAsmKind::CB_BRANCH: {
        if(InstAssembly_.size() >= 3) {
            if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rs1 >= 8U) orU16(c, (*rs1 - 8U) << 7); // CB rs1' in [9:7]
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
            orU16(c, (uint32_t { PAY.sub } >> 2) << 10);
            orU16(c, (uint32_t { PAY.sub } & 0x3U) << 5);
        }
    } break;
    case CAsmKind::CJ: {
        if(InstAssembly_.size() >= 2) {
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(1), imm)) encodeCjImm11(c, imm);
        }
    } break;
    case CAsmKind::CR: {
        if("c.ebreak" == MN) {
            orU16(c, 0x1U << 12);
        } else if("c.jr" == MN || "c.jalr" == MN) {
            if(InstAssembly_.size() >= 2) {
                if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rs1 } << 7);
            }
            if("c.jalr" == MN) orU16(c, 0x1U << 12);
        } else if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(2))) orU16(c, uint32_t { *rs2 } << 2);
            if("c.add" == MN) orU16(c, 0x1U << 12);
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
                const unsigned SCALE= ("c.sdsp" == MN || "c.fsdsp" == MN) ? 8U : 4U;
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
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiSpLoad(c, rd, imm, PAY.f3);
        }
    } break;
    default: break;
    }

    setCRaw(Layout_, c);
    mnemonicHelper();
    return &Layout_;
}

auto CType::calculateKeyID() -> KeyT
{
    Key_= resolveDecodeKey(Layout_);
    return Key_;
}

auto CType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
