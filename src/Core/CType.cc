#include "Core/CType.hh"

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

// Pack Layout_.C (op|b2) into the 16-bit RVC word used by scrambled-imm helpers.
[[nodiscard]] constexpr uint16_t C_RAW(const InstLayout &l) noexcept
{
    return l.C.b2 << 2 | l.C.opc;
}

// Write a 16-bit RVC word through Layout_.C; clears entity_[31:16].
void setC_RAW(InstLayout &l, uint16_t c) noexcept
{
    l.entity_= 0;
    l.C.opc  = c & 0x3;
    l.C.b2   = c >> 2 & 0x3FFF;
}

[[nodiscard]] constexpr uint8_t C_QUAD(const InstLayout &l) noexcept { return l.C.opc; }

[[nodiscard]] constexpr uint8_t C_F3(const InstLayout &l) noexcept { return l.CI.fct3; }

[[nodiscard]] constexpr uint8_t C_RD(const InstLayout &l) noexcept { return l.CI.rd; }

[[nodiscard]] constexpr uint8_t C_RS2(const InstLayout &l) noexcept { return l.CR.rs2; }

[[nodiscard]] constexpr uint8_t C_RD3(const InstLayout &l) noexcept { return l.CL.rd3; }

[[nodiscard]] constexpr uint8_t C_RS1_3(const InstLayout &l) noexcept { return l.CL.rs13; }

void orU16(uint16_t &c, uint32_t bits) noexcept { c|= bits; }

// Sign-extend imm[5:0]; cast keeps the right-shift arithmetic (signed).
[[nodiscard]] int32_t signExtend6(uint32_t imm6)
{
    return static_cast<int32_t>(imm6 << 26) >> 26; // unsigned shift avoids signed-overflow UB
}

[[nodiscard]] int32_t decodeCiImm6(uint16_t c)
{
    const uint32_t IMM= (c & 0x7C) >> 2 | (c & 0x1000) >> 7; // imm[4:0]@c[6:2], imm[5]@c[12]
    return signExtend6(IMM);
}

void encodeCiImm6(uint16_t &c, int32_t imm)
{
    const uint32_t U= static_cast<uint32_t>(imm) & 0x3F; // signed imm → unsigned field
    c&= ~((0x1FU << 2) | (0x1U << 12));                  // clear imm[4:0]@[6:2] and imm[5]@[12]
    orU16(c, (U & 0x1F) << 2 | (U >> 5 & 0x1) << 12);
}

// c.addi16sp: nzimm[9|4|6|8:7|5] → c[12|6|5|4:3|2]; returns signed byte offset.
[[nodiscard]] int32_t decodeAddi16SpImm(uint16_t c)
{
    const int32_t NZ= (c & 0x1000) >> 3 | (c & 0x40) >> 2 | (c & 0x20) << 1
                    | (c & 0x18) << 4 | (c & 0x4) << 3;
    return NZ << 22 >> 22;
}

void encodeAddi16SpImm(uint16_t &c, int32_t byteOff)
{
    const uint32_t U= static_cast<uint32_t>(byteOff);
    c&= ~((0x1FU << 2) | (0x1U << 12));
    orU16(c, (U >> 9 & 0x1) << 12 | (U >> 4 & 0x1) << 6 | (U >> 6 & 0x1) << 5 | (U >> 7 & 0x3) << 3 | (U >> 5 & 0x1) << 2);
}

// CSS: return byte offset. scale 4 → swsp/fswsp; scale 8 → sdsp/fsdsp.
[[nodiscard]] int32_t decodeCssUimm7(uint16_t c, uint32_t scale)
{
    if(8U == scale) { // sdsp/fsdsp: uimm[8:6]@c[9:7], uimm[5:3]@c[12:10]
        return (c & 0x1C00) >> 7 | (c & 0x380) >> 1;
    }
    return (c & 0x1E00) >> 7 | (c & 0x180) >> 1; // swsp/fswsp: uimm[5:2]@c[12:9], uimm[7:6]@c[8:7]
}

void encodeCssUimm7(uint16_t &c, int32_t byteOff, uint32_t scale)
{
    c&= ~(0x3FU << 7);
    const uint32_t U= static_cast<uint32_t>(byteOff);
    if(8U == scale) {
        orU16(c, (U >> 3 & 0x7) << 10 | (U >> 6 & 0x7) << 7);
    } else {
        orU16(c, (U >> 2 & 0xF) << 9 | (U >> 6 & 0x3) << 7);
    }
}

[[nodiscard]] int32_t decodeCiSpLoadImm(uint16_t c, uint32_t scale)
{
    if(8U == scale) { // ldsp: uimm[8:6]@c[4:2], uimm[4:3]@c[6:5], uimm[5]@c[12]
        return (c & 0x1000) >> 7 | (c & 0x60) >> 2 | (c & 0x1C) << 4;
    }
    return (c & 0x1000) >> 7 | (c & 0x70) >> 2 | (c & 0xC) << 4; // lwsp
}

[[nodiscard]] int32_t decodeClUimm(uint16_t c, uint32_t scale)
{
    if(8U == scale) { // ld/fld: uimm[7:6]@c[6:5], uimm[5:3]@c[12:10]
        return (c & 0x1C00) >> 7 | (c & 0x60) << 1;
    }
    return (c & 0x1C00) >> 7 | (c & 0x40) >> 4 | (c & 0x20) << 1; // lw/flw
}

[[nodiscard]] int32_t decodeCiwUimm(uint16_t c)
{
    return (c & 0x1800) >> 7 | (c & 0x780) >> 1 | (c & 0x40) >> 4 | (c & 0x20) >> 2;
}

void encodeCiSpLoad(uint16_t &c, uint8_t rd, int32_t byteOff, uint8_t f3, uint32_t scale)
{
    c&= 0x3;
    const uint32_t U= byteOff;
    const uint32_t B= (uint32_t { f3 } << 13) | (uint32_t { rd } << 7);
    if(8U == scale) {
        orU16(c, B | (U >> 5 & 0x1) << 12 | (U >> 3 & 0x3) << 5 | (U >> 6 & 0x7) << 2);
    } else {
        orU16(c, B | (U >> 5 & 0x1) << 12 | (U >> 2 & 0x7) << 4 | (U >> 6 & 0x3) << 2);
    }
}

void encodeClCs(uint16_t &c, uint8_t reg3, uint8_t rs1p, int32_t byteOff, uint8_t f3, uint32_t scale)
{
    c&= 0x3;
    const uint32_t U= byteOff;
    const uint32_t B= (uint32_t { f3 } << 13) | ((rs1p & 0x7) << 7) | ((reg3 & 0x7) << 2);
    if(8U == scale) {
        orU16(c, B | (U >> 3 & 0x7) << 10 | (U >> 6 & 0x3) << 5);
    } else {
        orU16(c, B | (U >> 3 & 0x7) << 10 | (U >> 2 & 0x1) << 6 | (U >> 6 & 0x1) << 5);
    }
}

void encodeCiwUimm(uint16_t &c, int32_t byteOff)
{
    const uint32_t NZ= byteOff;
    const uint8_t R3 = (c >> 2) & 0x7;
    c                = c & 0x3;
    orU16(c, (NZ >> 4 & 0x3) << 11 | (NZ >> 6 & 0xF) << 7 | (NZ >> 2 & 0x1) << 6 | (NZ >> 3 & 0x1) << 5 | (R3 & 0x7) << 2);
}

[[nodiscard]] int32_t decodeCbImm9(uint16_t c)
{
    // offset[8|4:3|7:6|2:1|5] → c[12|11:10|6:5|4:3|2]
    const auto IMM= (c & 0x1000) >> 4 | (c & 0xC00) >> 7 | (c & 0x60) << 1 | (c & 0x18) >> 2 | (c & 0x4) << 3;
    return IMM << 23 >> 23;
}

void encodeCbImm9(uint16_t &c, int32_t imm)
{
    const uint32_t S= imm & 0x1FF;
    c&= ~0x1C7CU;
    orU16(c, (S >> 5 & 0x1) << 2 | (S >> 1 & 0x3) << 3 | (S >> 6 & 0x3) << 5 | (S >> 3 & 0x3) << 10 | (S >> 8 & 0x1) << 12);
}

[[nodiscard]] int32_t decodeCjImm11(uint16_t c)
{
    // offset[11|4|9:8|10|6|7|3:1|5] → c[12|11|10:9|8|7|6|5:3|2]
    const auto IMM= (c & 0x1000) >> 1 | (c & 0x800) >> 7 | (c & 0x600) >> 1 | (c & 0x100) << 2
                  | (c & 0x80) >> 1 | (c & 0x40) << 1 | (c & 0x38) >> 2 | (c & 0x4) << 3;
    return IMM << 20 >> 20;
}

void encodeCjImm11(uint16_t &c, int32_t imm)
{
    const uint32_t S= imm & 0xFFF;
    c&= ~0x1FFEU;

    orU16(c, (S >> 5 & 0x1) << 2);
    orU16(c, (S >> 1 & 0x7) << 3);
    orU16(c, (S >> 7 & 0x1) << 6);
    orU16(c, (S >> 6 & 0x1) << 7);
    orU16(c, (S >> 10 & 0x1) << 8);
    orU16(c, (S >> 8 & 0x3) << 9);
    orU16(c, (S >> 4 & 0x1) << 11);
    orU16(c, (S >> 11 & 0x1) << 12);
}

[[nodiscard]] uint8_t calcSubFromInst(const InstLayout &l)
{
    const uint8_t Q = C_QUAD(l);
    const uint8_t F3= C_F3(l);

    // Q = inst[1:0] quadrant; F3 = inst[15:13] funct3 (RVC Ch.27).
    switch(Q) {
        // Q0: CIW/CL/CS. F3=3/7 are RV32FC flw/fsw vs RV64C ld/sd aliases;
        // prefer FP (sub=0) like rvcodec AUTO. All other F3 also use sub=0.
    case 0: return 0;
    case 1: // Quadrant 1: CI / CB / CA / CJ
        switch(F3) {
        case 0:                                                          // c.addi; rd=0 & imm=0 → c.nop (sub=1)
            return 0U == C_RD(l) && 0 == decodeCiImm6(C_RAW(l)) ? 1 : 0; // c.nop : c.addi
        case 1:                                                          // c.addiw (RV64 CI) vs c.jal (RV32 CJ) — prefer c.jal (sub=1) like rvcodec AUTO
            return 1;
        case 3:                           // c.lui; rd=x2 → c.addi16sp (sub=1)
            return 2U == C_RD(l) ? 1 : 0; // c.addi16sp : c.lui
        case 4: {                         // misc-ALU: CB shift/andi or CA — use [11:10]; bit12 is shamt[5] for shifts
            const uint8_t F11_10= C_RAW(l) >> 10 & 0x3;
            const uint8_t F2    = l.CA.fct2;
            switch(F11_10) {
            case 0:  return 0; // c.srli
            case 1:  return 1; // c.srai
            case 2:  return 8; // c.andi
            default: {         // CA: [12:10]=011 sub* / 111 *w
                const uint8_t B12= C_RAW(l) >> 12 & 0x1;
                return (B12 != 0U) ? (0x07U << 2) | F2  // subw/addw
                                   : (0x03U << 2) | F2; // sub/xor/or/and
            }
            }
        }
        default: return 0;
        }
    case 2:           // Quadrant 2: CI / CR / CSS (F3=3/7 flwsp/fswsp vs ldsp/sdsp → sub=0)
        if(4 == F3) { // CR family: jr/mv/ebreak/jalr/add via bit12 + rs2
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
        return 0;
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
    return { .f3_ = static_cast<uint8_t>(PAY >> 8 & 0x7),
             .sub_= static_cast<uint8_t>(PAY >> 2 & 0x3F),
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

[[nodiscard]] uint32_t byteScale(std::string_view nm) noexcept
{
    // Double-width RVC loads/stores (incl. *sp): ld/sd/fld/fsd family.
    return nm.starts_with("c.ld") || nm.starts_with("c.sd") || nm.starts_with("c.fld") || nm.starts_with("c.fsd")
             ? 8U
             : 4U;
}

[[nodiscard]] std::optional<uint8_t> parseReg3(std::string_view tok, bool fp)
{
    if(fp) {
        if(auto i= isa::LookupFregIdx(tok); i && *i >= 8U) return static_cast<uint8_t>(*i - 8U);
    } else if(auto i= isa::LookupRegIdx(tok); i && *i >= 8U) {
        return static_cast<uint8_t>(*i - 8U);
    }
    return std::nullopt;
}

void orReg3At(uint16_t &c, std::string_view tok, unsigned shift)
{
    if(auto r= parseReg3(tok, false)) orU16(c, uint32_t { *r } << shift);
}

void parseClCsOperands(const std::vector<std::string> &parts, bool fp, uint8_t &r3, uint8_t &rs1p, int32_t &imm)
{
    r3= rs1p= 0;
    imm     = 0;
    if(parts.size() >= 4) {
        if(auto r= parseReg3(parts[1], fp)) r3= *r;
        if(auto r= parseReg3(parts[3], false)) rs1p= *r;
        (void)util::ParseInt(parts[2], imm);
    } else if(parts.size() >= 3) {
        if(auto r= parseReg3(parts[1], fp)) r3= *r;
        std::string base;
        if(util::ParseMemOperand(parts[2], imm, base)) {
            if(auto r= parseReg3(base, false)) rs1p= *r;
        }
    }
}

void encodeCiRdImm(uint16_t &c, const std::vector<std::string> &parts)
{
    if(parts.size() < 3) return;
    if(auto rd= isa::LookupRegIdx(parts[1])) orU16(c, uint32_t { *rd } << 7);
    int32_t imm {};
    if(util::ParseInt(parts[2], imm)) encodeCiImm6(c, imm);
}

void encodeShamt6(uint16_t &c, int32_t sh)
{
    orU16(c, (sh & 0x1F) << 2);
    orU16(c, (sh >> 5 & 0x1) << 12);
}

[[nodiscard]] CAsmKind GetkindFromKey(CType::KeyT key)
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
        case 1:  return 1 == P.sub_ ? CAsmKind::CJ : CAsmKind::CI; // c.jal (RV32) / c.addiw (RV64)
        default: return CAsmKind::CI;                              // c.addi / c.li / c.nop
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
    case CShape::CR:  setBitsField({ Layout_.CR.opc, Layout_.CR.rs2, Layout_.CR.rd, Layout_.CR.fct4 }); break;
    case CShape::CI:  setBitsField({ Layout_.CI.opc, Layout_.CI.imm0t4, Layout_.CI.rd, Layout_.CI.imm5, Layout_.CI.fct3 }); break;
    case CShape::CSS: setBitsField({ Layout_.CSS.opc, Layout_.CSS.rs2, Layout_.CSS.imm6, Layout_.CSS.fct3 }); break;
    case CShape::CIW: setBitsField({ Layout_.CIW.opc, Layout_.CIW.rd3, Layout_.CIW.imm8, Layout_.CIW.fct3 }); break;
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
    case CShape::CA: setBitsField({ Layout_.CA.opc, Layout_.CA.rs23, Layout_.CA.fct2, Layout_.CA.rd3, Layout_.CA.fct6 }); break;
    case CShape::CB: setBitsField({ Layout_.CB.opc, Layout_.CB.offLo, Layout_.CB.rd3, Layout_.CB.offHi, Layout_.CB.fct3 }); break;
    case CShape::CJ: setBitsField({ Layout_.CJ.opc, Layout_.CJ.target, Layout_.CJ.fct3 }); break;
    }
}

void CType::mnemonicHelper()
{
    const std::string_view NM= InstAssembly_.empty()
                                 ? LookupNameAndInfo().name_
                                 : InstAssembly_.front();

    const auto RAW  = C_RAW(Layout_);
    const auto KIND = GetkindFromKey(Key_);
    const auto SCALE= byteScale(NM);

    if("c.nop" == NM || "c.ebreak" == NM) {
        if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
        return;
    }

    switch(KIND) {
    case CAsmKind::CIW_ADDI4SPN: appendOperands({ reg3Name(C_RD3(Layout_), HasSetABI_), ",", std::to_string(decodeCiwUimm(RAW)) }); break;
    case CAsmKind::CL_INT:
    case CAsmKind::CL_FP:
    case CAsmKind::CS_INT:
    case CAsmKind::CS_FP:        {
        const bool FP= CAsmKind::CL_FP == KIND || CAsmKind::CS_FP == KIND;
        const auto R = FP ? freg3Name(C_RD3(Layout_), HasSetABI_) : reg3Name(C_RD3(Layout_), HasSetABI_);
        appendOperands({ R, ",", std::to_string(decodeClUimm(RAW, SCALE)), "(", reg3Name(C_RS1_3(Layout_), HasSetABI_), ")" });
    } break;

    case CAsmKind::CI:
    case CAsmKind::CI_LUI:      appendOperands({ isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) }); break;
    case CAsmKind::CI_ADDI16SP: appendOperands({ std::to_string(decodeAddi16SpImm(RAW)) }); break;
    case CAsmKind::CB_SHIFT:    {
        const uint32_t SH= Layout_.CI.imm0t4 | Layout_.CI.imm5 << 5;
        appendOperands({ reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(SH) }); // CB.rd3 @ [9:7]
    } break;

    case CAsmKind::CB_ANDI:   appendOperands({ reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(decodeCiImm6(RAW)) }); break;
    case CAsmKind::CB_BRANCH: appendOperands({ reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", std::to_string(decodeCbImm9(RAW)) }); break;
    case CAsmKind::CA:        appendOperands({ reg3Name(C_RS1_3(Layout_), HasSetABI_), ",", reg3Name(C_RD3(Layout_), HasSetABI_) }); break;
    case CAsmKind::CJ:        appendOperands({ std::to_string(decodeCjImm11(RAW)) }); break;
    case CAsmKind::CR:        {
        if("c.jr" == NM || "c.jalr" == NM) {
            appendOperands({ isa::LookupRegName(C_RD(Layout_), HasSetABI_) });
        } else if("c.mv" == NM || "c.add" == NM) {
            appendOperands({ isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", isa::LookupRegName(C_RS2(Layout_), HasSetABI_) });
        }
    } break;

    case CAsmKind::CSS_INT:
    case CAsmKind::CSS_FP:  {
        const auto R= CAsmKind::CSS_FP == KIND ? isa::LookupFregName(C_RS2(Layout_), HasSetABI_)
                                               : isa::LookupRegName(C_RS2(Layout_), HasSetABI_);
        appendOperands({ R, ",", std::to_string(decodeCssUimm7(RAW, SCALE)) });
    } break;

    case CAsmKind::CI_SLLI: {
        const uint32_t SH= Layout_.CI.imm0t4 | Layout_.CI.imm5 << 5;
        appendOperands({ isa::LookupRegName(C_RD(Layout_), HasSetABI_), ",", std::to_string(SH) });
    } break;

    case CAsmKind::CI_SP_LOAD:
    case CAsmKind::CI_SP_FP_LOAD: {
        const auto R= CAsmKind::CI_SP_FP_LOAD == KIND ? isa::LookupFregName(C_RD(Layout_), HasSetABI_)
                                                      : isa::LookupRegName(C_RD(Layout_), HasSetABI_);
        appendOperands({ R, ",", std::to_string(decodeCiSpLoadImm(RAW, SCALE)) });
    } break;

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

    const auto &mn      = InstAssembly_.at(0);
    const CAsmKind KIND = GetkindFromKey(info.keyID_);
    const uint32_t SCALE= byteScale(mn);

    switch(KIND) {
    case CAsmKind::CIW_ADDI4SPN: {
        if(InstAssembly_.size() >= 3) {
            orReg3At(c, InstAssembly_.at(1), 2);
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiwUimm(c, imm);
        }
    } break;

    case CAsmKind::CL_INT:
    case CAsmKind::CL_FP:
    case CAsmKind::CS_INT:
    case CAsmKind::CS_FP:  {
        const bool FP= CAsmKind::CL_FP == KIND || CAsmKind::CS_FP == KIND;
        uint8_t r3 {};
        uint8_t rs1p {};
        int32_t imm {};
        parseClCsOperands(InstAssembly_, FP, r3, rs1p, imm);
        encodeClCs(c, r3, rs1p, imm, PAY.f3_, SCALE);
    } break;

    case CAsmKind::CI:
    case CAsmKind::CI_LUI:      encodeCiRdImm(c, InstAssembly_); break;
    case CAsmKind::CI_ADDI16SP: {
        orU16(c, 0x2U << 7);
        if(InstAssembly_.size() >= 2) {
            int32_t imm {};
            const std::string_view IMM_TOK= InstAssembly_.size() >= 3 ? InstAssembly_.at(2)
                                                                      : InstAssembly_.at(1);
            if(util::ParseInt(IMM_TOK, imm)) encodeAddi16SpImm(c, imm);
        }
    } break;

    case CAsmKind::CB_SHIFT: {
        if(InstAssembly_.size() >= 3) {
            orReg3At(c, InstAssembly_.at(1), 7); // CB rd' in [9:7]
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) encodeShamt6(c, sh);
            if("c.srai" == mn) orU16(c, 0x1U << 10); // [11:10]=01
        }
    } break;

    case CAsmKind::CB_ANDI: {
        if(InstAssembly_.size() >= 3) {
            orReg3At(c, InstAssembly_.at(1), 7); // CB rd' in [9:7]
            orU16(c, 0x2U << 10);                // funct3=100, [12:10]=010 for C.ANDI
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiImm6(c, imm);
        }
    } break;

    case CAsmKind::CB_BRANCH: {
        if(InstAssembly_.size() >= 3) {
            orReg3At(c, InstAssembly_.at(1), 7); // CB rs1' in [9:7]
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCbImm9(c, imm);
        }
    } break;

    case CAsmKind::CA: {
        if(InstAssembly_.size() >= 3) {
            orReg3At(c, InstAssembly_.at(1), 7);
            orReg3At(c, InstAssembly_.at(2), 2);
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
            if(CAsmKind::CSS_FP == KIND) {
                if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rs2 } << 2);
            } else if(auto rs2= isa::LookupRegIdx(InstAssembly_.at(1))) {
                orU16(c, uint32_t { *rs2 } << 2);
            }
            int32_t imm {};
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCssUimm7(c, imm, SCALE);
        }
    } break;

    case CAsmKind::CI_SLLI: {
        if(InstAssembly_.size() >= 3) {
            if(auto rd= isa::LookupRegIdx(InstAssembly_.at(1))) orU16(c, uint32_t { *rd } << 7);
            int32_t sh {};
            if(util::ParseInt(InstAssembly_.at(2), sh)) encodeShamt6(c, sh);
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
            if(util::ParseInt(InstAssembly_.at(2), imm)) encodeCiSpLoad(c, rd, imm, PAY.f3_, SCALE);
        }
    } break;

    default: break;
    }

    setC_RAW(Layout_, c);
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
