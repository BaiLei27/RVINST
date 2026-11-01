#include "Core/CType.hh"

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

// Pack Layout_.C (op|b2) into the 16-bit RVC word used by scrambled-imm helpers.
[[nodiscard]] constexpr uint16_t C_RAW(const InstLayout &l) noexcept
{
    return (l.C.b2 << 2) | l.C.opc;
}

// Write a 16-bit RVC word through Layout_.C; clears entity_[31:16].
void setCRaw(InstLayout &l, uint16_t c) noexcept
{
    l.entity_= 0;
    l.C.opc  = c & 0x3;
    l.C.b2   = (c >> 2) & 0x3FFF;
}

[[nodiscard]] constexpr uint8_t C_QUAD(const InstLayout &l) noexcept { return l.C.opc; }

[[nodiscard]] constexpr uint8_t C_F3(const InstLayout &l) noexcept { return l.CI.fct3; }

[[nodiscard]] constexpr uint8_t C_RD(const InstLayout &l) noexcept { return l.CI.rd; }

[[nodiscard]] constexpr uint8_t C_RS2(const InstLayout &l) noexcept { return l.CR.rs2; }

[[nodiscard]] constexpr uint8_t C_RD3(const InstLayout &l) noexcept { return l.CL.rd3; }

[[nodiscard]] constexpr uint8_t C_RS1_3(const InstLayout &l) noexcept { return l.CL.rs13; }

void orU16(uint16_t &c, uint32_t bits) noexcept { c= c | bits; }

// Sign-extend imm[5:0]; cast is required so the arithmetic right-shift is signed.
[[nodiscard]] int32_t signExtend6(uint32_t imm6)
{
    const int32_t S= static_cast<int32_t>(imm6);
    return (S << 26) >> 26;
}

[[nodiscard]] int32_t decodeCiImm6(uint16_t c)
{
    const uint32_t IMM= (c >> 12 & 0x1) << 5 | (c >> 2 & 0x1F);
    return signExtend6(IMM);
}

void encodeCiImm6(uint16_t &c, int32_t imm)
{
    const uint32_t U= static_cast<uint32_t>(imm) & 0x3F; // signed imm → unsigned field
    c&= ~((0x1FU << 2) | (0x1U << 12));                  // U only on << LHS
    orU16(c, (U & 0x1F) << 2);
    orU16(c, (U >> 5 & 0x1) << 12);
}

[[nodiscard]] int32_t decodeCssUimm7(uint16_t c, uint32_t scale)
{
    return static_cast<int32_t>(((c >> 7) & 0x7F) * scale);
}

void encodeCssUimm7(uint16_t &c, int32_t byteOff, uint32_t scale)
{
    const uint32_t UIMM= byteOff / static_cast<int32_t>(scale) & 0x3F; // signed byte÷scale
    c&= ~(0x3FU << 7);                                                 // CSS uimm[5:0] in [12:7]; keep funct3[15:13]
    orU16(c, UIMM << 7);
}

[[nodiscard]] int32_t decodeCiSpLoadImm(uint16_t c)
{
    const uint32_t UIMM= (c >> 12 & 0x1) << 5 | (c >> 2 & 0x7) << 2 | (c >> 5 & 0x3) << 6
                       | (c >> 4 & 0x4) << 6;
    return static_cast<int32_t>(UIMM * 4);
}

// CL/CS uimm[7:2] (byte offset / scale); rd'/rs2' in [4:2], rs1' in [9:7].
[[nodiscard]] int32_t decodeClUimm(uint16_t c, uint32_t scale)
{
    const uint32_t UIMM= (c >> 10 & 0x7) << 3 | (c >> 5 & 0x3) << 6 | (c >> 2 & 0x7) << 2
                       | (c >> 12 & 0x1) << 5;
    return static_cast<int32_t>(UIMM * scale);
}

// CIW nzuimm[9:2] (byte offset, multiple of 4); rd' in [4:2].
[[nodiscard]] int32_t decodeCiwUimm(uint16_t c)
{
    const uint32_t NZ= (c >> 12 & 0x1) << 5 | (c >> 7 & 0xF) << 2 | (c >> 6 & 0x1) << 6
                     | (c >> 5 & 0x1) << 5 | (c >> 2 & 0x3) | (c >> 11 & 0x3) << 4;
    return static_cast<int32_t>(NZ * 4);
}

void encodeCiSpLoad(uint16_t &c, uint8_t rd, int32_t byteOff, uint8_t f3)
{
    c&= 0x3;
    orU16(c, uint32_t { f3 } << 13);
    orU16(c, uint32_t { rd } << 7);
    const uint32_t UIMM= byteOff / 4 & 0x3F;
    orU16(c, (UIMM >> 5 & 0x1) << 12);
    orU16(c, (UIMM >> 2 & 0x7) << 2);
    orU16(c, (UIMM >> 4 & 0x1) << 4);
    orU16(c, (UIMM >> 6 & 0x3) << 5);
}

void encodeClCs(uint16_t &c, uint8_t reg3, uint8_t rs1p, uint32_t uimm, uint8_t f3)
{
    c&= 0x3;
    orU16(c, uint32_t { f3 } << 13);
    orU16(c, (uimm >> 5 & 0x1) << 12);
    orU16(c, (uimm >> 3 & 0x3) << 10);
    orU16(c, (rs1p & 0x7) << 7);
    orU16(c, (uimm >> 6 & 0x3) << 5);
    orU16(c, (reg3 & 0x7) << 2);
}

void encodeCiwUimm(uint16_t &c, int32_t byteOff)
{
    const uint32_t NZ= byteOff / 4 & 0xFF;
    const uint8_t R3 = (c >> 2) & 0x7;
    c                = c & 0x3;
    orU16(c, (NZ >> 6 & 0x1) << 12);
    orU16(c, (NZ >> 4 & 0x3) << 11);
    orU16(c, (NZ >> 2 & 0xF) << 7);
    orU16(c, (NZ >> 6 & 0x1) << 6);
    orU16(c, (NZ >> 5 & 0x1) << 5);
    orU16(c, ((R3 & 0x7) << 2) | (NZ & 0x3));
}

[[nodiscard]] int32_t decodeCbImm9(uint16_t c)
{
    const auto IMM= static_cast<int32_t>((c >> 12 & 0x1) << 8 | (c >> 5 & 0x3) << 6 | (c >> 2 & 0x7) << 3
                                         | (c >> 10 & 0x3) << 1 | (c >> 12 & 0x1));
    return IMM << 23 >> 23;
}

void encodeCbImm9(uint16_t &c, int32_t imm)
{
    const int32_t S= imm & 0x1FF;
    c&= ~0x1F82U; // U: unsigned ~ so high bits stay 1 before truncate into uint16_t
    orU16(c, (S >> 3 & 0x7) << 2);
    orU16(c, (S >> 6 & 0x3) << 5);
    orU16(c, (S >> 1 & 0x3) << 10);
    orU16(c, (S >> 8 & 0x1) << 12);
}

[[nodiscard]] int32_t decodeCjImm11(uint16_t c)
{
    const auto IMM= static_cast<int32_t>((c >> 12 & 0x1) << 10 | (c >> 2 & 0x1F) << 5 | (c >> 7 & 0x1F) << 1
                                         | (c >> 11 & 0x1) << 4 | (c >> 9 & 0x3) << 8 | (c >> 8 & 0x1) << 3
                                         | (c >> 10 & 0x1) << 2);
    return IMM << 21 >> 21;
}

void encodeCjImm11(uint16_t &c, int32_t imm)
{
    const int32_t S= imm & 0x7FF;
    c&= ~0x1FFEU; // U: unsigned ~
    orU16(c, (S >> 1 & 0x1F) << 7);
    orU16(c, (S >> 5 & 0x1F) << 2);
    orU16(c, (S >> 10 & 0x1) << 12);
    orU16(c, (S >> 11 & 0x1) << 11);
    orU16(c, (S >> 9 & 0x1) << 8);
    orU16(c, (S >> 7 & 0x3) << 9);
    orU16(c, (S >> 4 & 0x1) << 11);
}

[[nodiscard]] uint8_t calcSubFromInst(const InstLayout &l)
{
    const uint8_t Q = C_QUAD(l);
    const uint8_t F3= C_F3(l);

    // Q = inst[1:0] quadrant; F3 = inst[15:13] funct3 (RVC Ch.27).
    switch(Q) {
    case 0: // Quadrant 0: CIW / CL / CS
        switch(F3) {
        case 3:            // c.flw / c.ld — sub=1 selects LD over FLW
        case 7:  return 1; // c.fsw / c.sd — sub=1 selects SD over FSW
        default: return 0;
        }
    case 1: // Quadrant 1: CI / CB / CA / CJ
        switch(F3) {
        case 0:                                                          // c.addi; rd=0 & imm=0 → c.nop (sub=1)
            return 0U == C_RD(l) && 0 == decodeCiImm6(C_RAW(l)) ? 1 : 0; // c.nop : c.addi
        case 1: return 0;                                                // c.addiw (RV64) / c.jal (RV32); sub unused
        case 3:                                                          // c.lui; rd=x2 → c.addi16sp (sub=1)
            return 2U == C_RD(l) ? 1 : 0;                                // c.addi16sp : c.lui
        case 4: {                                                        // misc-ALU: CB shift/andi or CA
            const uint8_t F610= l.CA.fct6 & 0x7;                         // inst[12:10]
            const uint8_t F2  = l.CA.fct2;
            switch(F610) {
            case 7:  return (0x07U << 2) | F2;      // CA: 100 11 x — sub/xor/or/and
            case 3:  return (0x03U << 2) | F2;      // CA: 100 01 x — subw/addw
            case 2:  return 8;                      // CB: 100 10 — c.andi (sub=8)
            default: return 0 != l.CI.imm5 ? 1 : 0; // CB: c.srai (sub=1) / c.srli (sub=0)
            }
        }
        default: return 0;
        }
    case 2: // Quadrant 2: CI / CR / CSS
        switch(F3) {
        case 3:           // c.flwsp / c.ldsp — sub=1 selects LDSP
        case 7: return 1; // c.fswsp / c.sdsp — sub=1 selects SDSP
        case 4: {         // CR family: jr/mv/ebreak/jalr/add via bit12 + rs2
            const uint8_t B12= l.CI.imm5;
            const uint8_t RS2= C_RS2(l);
            if(0U == B12) {
                return 0U == RS2 ? 0 : 4; // JR : MV
            }
            if(0U == RS2) {
                return 0U == C_RD(l) ? 2 : 1; // EBREAK : JALR
            }
            return 3; // ADD
        }
        default: return 0;
        }
    default: return 0;
    }
}

struct CPay {
    uint8_t f3_ {};
    uint8_t sub_ {};
    uint8_t q_ {};
};

[[nodiscard]] constexpr CPay UNPACK_PAY(CType::KeyT key) noexcept
{
    const CType::KeyT PAY= CType::KEY_PAYLOAD(key);
    return { .f3_ = static_cast<uint8_t>((PAY >> 8) & 0x7),
             .sub_= static_cast<uint8_t>((PAY >> 2) & 0x3F),
             .q_  = static_cast<uint8_t>(PAY & 0x3) };
}

[[nodiscard]] bool instMatchesKey(const InstLayout &l, CType::KeyT key)
{
    const CPay EXP= UNPACK_PAY(key);
    return C_QUAD(l) == EXP.q_ && C_F3(l) == EXP.f3_ && calcSubFromInst(l) == EXP.sub_;
}

[[nodiscard]] CType::KeyT resolveDecodeKey(const InstLayout &l)
{
    for(const auto &entry: CType::G_INST_TABLE) {
        if(instMatchesKey(l, entry.keyID_)) return entry.keyID_;
    }
    return 0xffff;
}

[[nodiscard]] std::string reg3Name(uint8_t r3, bool abi)
{
    return std::string(isa::LookupRegName(8 + r3, abi));
}

[[nodiscard]] std::string freg3Name(uint8_t r3, bool abi)
{
    return std::string(isa::LookupFregName(8 + r3, abi));
}

[[nodiscard]] CAsmKind kindFromKey(CType::KeyT key)
{
    const CPay P= UNPACK_PAY(key);

    // q_ = inst[1:0]; f3_ = inst[15:13] (same encoding as calcSubFromInst).
    switch(P.q_) {
    case 0: // Quadrant 0
        switch(P.f3_) {
        case 0:  return CAsmKind::CIW_ADDI4SPN;                           // c.addi4spn
        case 1:  return CAsmKind::CL_FP;                                  // c.fld
        case 5:  return CAsmKind::CS_FP;                                  // c.fsd
        case 2:  return CAsmKind::CL_INT;                                 // c.lw
        case 6:  return CAsmKind::CS_INT;                                 // c.sw
        case 3:  return 0 == P.sub_ ? CAsmKind::CL_FP : CAsmKind::CL_INT; // c.flw / c.ld
        case 7:  return 0 == P.sub_ ? CAsmKind::CS_FP : CAsmKind::CS_INT; // c.fsw / c.sd
        default: break;
        }
        break;
    case 1: // Quadrant 1
        switch(P.f3_) {
        case 5: return CAsmKind::CJ;        // c.j
        case 6:                             // c.beqz
        case 7: return CAsmKind::CB_BRANCH; // c.bnez
        case 4:                             // misc-ALU
            if(8U == P.sub_) return CAsmKind::CB_ANDI;
            if(P.sub_ <= 1U) return CAsmKind::CB_SHIFT; // c.srli / c.srai
            return CAsmKind::CA;
        case 3:  return 1 == P.sub_ ? CAsmKind::CI_ADDI16SP : CAsmKind::CI_LUI;
        default: return CAsmKind::CI; // c.addi / c.li / c.nop / c.addiw / c.jal
        }
    case 2: // Quadrant 2
        switch(P.f3_) {
        case 0:  return CAsmKind::CI_SLLI;                                            // c.slli
        case 1:  return CAsmKind::CI_SP_FP_LOAD;                                      // c.fldsp
        case 5:  return CAsmKind::CSS_FP;                                             // c.fsdsp
        case 2:  return CAsmKind::CI_SP_LOAD;                                         // c.lwsp
        case 6:  return CAsmKind::CSS_INT;                                            // c.swsp
        case 3:  return 0 == P.sub_ ? CAsmKind::CI_SP_FP_LOAD : CAsmKind::CI_SP_LOAD; // c.flwsp / c.ldsp
        case 4:  return CAsmKind::CR;                                                 // c.jr / c.mv / c.ebreak / c.jalr / c.add
        case 7:  return 0 == P.sub_ ? CAsmKind::CSS_FP : CAsmKind::CSS_INT;           // c.fswsp / c.sdsp
        default: break;
        }
        break;
    default: break;
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
        const uint32_t SCALE= "c.ld" == NM ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CL_FP: {
        const auto RD       = freg3Name(C_RD3(Layout_), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const uint32_t SCALE= ("c.fld" == NM || "c.fldsp" == NM) ? 8U : 4U;
        appendOperands({ " ", RD, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_INT: {
        const auto RS2      = reg3Name(C_RD3(Layout_), HasSetABI_); // CS.rs23 @ [4:2]
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const uint32_t SCALE= "c.sd" == NM ? 8U : 4U;
        appendOperands({ " ", RS2, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", RS1, ")" });
    } break;
    case CAsmKind::CS_FP: {
        const auto RS2      = freg3Name(C_RD3(Layout_), HasSetABI_);
        const auto RS1      = reg3Name(C_RS1_3(Layout_), HasSetABI_);
        const uint32_t SCALE= ("c.fsd" == NM || "c.fsdsp" == NM) ? 8U : 4U;
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
        appendOperands({ " ", reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(SH) }); // CB.rd3 @ [9:7]
    } break;
    case CAsmKind::CB_ANDI:
        appendOperands({ " ", reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) });
        break;
    case CAsmKind::CB_BRANCH:
        appendOperands({ " ", reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(decodeCbImm9(RAW)) });
        break;
    case CAsmKind::CA:
        appendOperands({ " ", reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", reg3Name(C_RD3(Layout_), HasSetABI_) });
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
        const uint32_t SCALE= "c.sdsp" == NM ? 8U : 4U;
        appendOperands({ " ", isa::LookupRegName(C_RS2(Layout_), HasSetABI_), ",", std::to_string(decodeCssUimm7(RAW, SCALE)) });
    } break;
    case CAsmKind::CSS_FP: {
        const uint32_t SCALE= "c.fsdsp" == NM ? 8U : 4U;
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
                    if(auto i= isa::LookupFregIdx(tok)) return *i - 8;
                } else if(auto i= isa::LookupRegIdx(tok)) {
                    return *i >= 8 ? std::optional<uint8_t> { *i - 8 } : std::nullopt;
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
                const uint32_t UIMM = imm / static_cast<int32_t>(SCALE);
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
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(1))) rs2p= *rs2 - 8U;
            } else if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(1))) {
                if(*rs2 >= 8U) rs2p= *rs2 - 8U;
            }
            if(auto rs1= isa::LookupRegIdx(InstAssembly_.at(3))) {
                if(*rs1 >= 8U) rs1p= *rs1 - 8U;
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) {
                const uint32_t SCALE= ("c.sd" == mn || "c.fsd" == mn) ? 8U : 4U;
                const uint32_t UIMM = imm / static_cast<int32_t>(SCALE);
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
                if(*rd >= 8U) orU16(c, (*rd - 8U) << 7); // CB rd' in [9:7]
            }
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) {
                orU16(c, (sh & 0x1F) << 2);
                orU16(c, (sh >> 5 & 0x1) << 12);
            }
            if("c.srai" == mn) orU16(c, 0x1U << 12);
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
            orU16(c, (uint32_t { PAY.sub_ } >> 2) << 10);
            orU16(c, (uint32_t { PAY.sub_ } & 0x3) << 5);
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
                orU16(c, (sh & 0x1F) << 2);
                orU16(c, (sh >> 5 & 0x1) << 12);
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
