#include <array>
#include <utility>

#include "ISA/InstFormat.hh"

namespace {

using namespace std::string_view_literals;

// startBit_ = LSB, endBit_ = MSB (BinaryFieldWidget: width = end - start + 1).
constexpr std::array<InstField, 6> G_R_FIELDS {
    {
     { .name_= "funct7"sv, .startBit_= 25, .endBit_= 31, .desc_= "funct7"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 8> G_R_AMO_FIELDS {
    {
     { .name_= "funct5"sv, .startBit_= 27, .endBit_= 31, .desc_= "funct5"sv },
     { .name_= "aq"sv, .startBit_= 26, .endBit_= 26, .desc_= "aq"sv },
     { .name_= "rl"sv, .startBit_= 25, .endBit_= 25, .desc_= "rl"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 7> G_R_FP_FIELDS {
    {
     { .name_= "funct5"sv, .startBit_= 27, .endBit_= 31, .desc_= "funct5"sv },
     { .name_= "fmt"sv, .startBit_= 25, .endBit_= 26, .desc_= "fmt"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "rm"sv, .startBit_= 12, .endBit_= 14, .desc_= "rm / funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 7> G_R_R4_FIELDS {
    {
     { .name_= "rs3"sv, .startBit_= 27, .endBit_= 31, .desc_= "rs3"sv },
     { .name_= "fmt"sv, .startBit_= 25, .endBit_= 26, .desc_= "fmt"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "rm"sv, .startBit_= 12, .endBit_= 14, .desc_= "rm"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

// Indexed by RShape (BASE=0 .. R4=3).
constexpr std::array<std::span<const InstField>, 4> G_R_SHAPE_FIELDS {
    std::span<const InstField>(G_R_FIELDS),
    std::span<const InstField>(G_R_AMO_FIELDS),
    std::span<const InstField>(G_R_FP_FIELDS),
    std::span<const InstField>(G_R_R4_FIELDS),
};

constexpr std::array<InstField, 5> G_I_FIELDS {
    {
     { .name_= "imm11_0"sv, .startBit_= 20, .endBit_= 31, .desc_= "imm[11:0]"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 6> G_I_SHIFT_FIELDS {
    {
     { .name_= "imm7"sv, .startBit_= 25, .endBit_= 31, .desc_= "imm7"sv },
     { .name_= "shamt"sv, .startBit_= 20, .endBit_= 24, .desc_= "shamt"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 7> G_I_FENCE_FIELDS {
    {
     { .name_= "fm"sv, .startBit_= 28, .endBit_= 31, .desc_= "fm"sv },
     { .name_= "pred"sv, .startBit_= 24, .endBit_= 27, .desc_= "pred"sv },
     { .name_= "succ"sv, .startBit_= 20, .endBit_= 23, .desc_= "succ"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 5> G_I_CSR_FIELDS {
    {
     { .name_= "csr"sv, .startBit_= 20, .endBit_= 31, .desc_= "csr"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1 / uimm"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 6> G_S_FIELDS {
    {
     { .name_= "imm11_5"sv, .startBit_= 25, .endBit_= 31, .desc_= "imm[11:5]"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "imm4_0"sv, .startBit_= 7, .endBit_= 11, .desc_= "imm[4:0]"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 8> G_B_FIELDS {
    {
     { .name_= "imm12"sv, .startBit_= 31, .endBit_= 31, .desc_= "imm[12]"sv },
     { .name_= "imm10_5"sv, .startBit_= 25, .endBit_= 30, .desc_= "imm[10:5]"sv },
     { .name_= "rs2"sv, .startBit_= 20, .endBit_= 24, .desc_= "rs2"sv },
     { .name_= "rs1"sv, .startBit_= 15, .endBit_= 19, .desc_= "rs1"sv },
     { .name_= "funct3"sv, .startBit_= 12, .endBit_= 14, .desc_= "funct3"sv },
     { .name_= "imm4_1"sv, .startBit_= 8, .endBit_= 11, .desc_= "imm[4:1]"sv },
     { .name_= "imm11"sv, .startBit_= 7, .endBit_= 7, .desc_= "imm[11]"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 3> G_U_FIELDS {
    {
     { .name_= "imm31_12"sv, .startBit_= 12, .endBit_= 31, .desc_= "imm[31:12]"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

constexpr std::array<InstField, 2> G_C_FIELDS {
    {
     { .name_= "inst15_2"sv, .startBit_= 2, .endBit_= 15, .desc_= "inst[15:2]"sv },
     { .name_= "quadrant"sv, .startBit_= 0, .endBit_= 1, .desc_= "inst[1:0]"sv },
     }
};

constexpr std::array<InstField, 4> G_C_CR_FIELDS {
    {
     { .name_= "funct4"sv, .startBit_= 12, .endBit_= 15, .desc_= "funct4"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd/rs1"sv },
     { .name_= "rs2"sv, .startBit_= 2, .endBit_= 6, .desc_= "rs2"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 5> G_C_CI_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "imm5"sv, .startBit_= 12, .endBit_= 12, .desc_= "imm"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd/rs1"sv },
     { .name_= "imm0t4"sv, .startBit_= 2, .endBit_= 6, .desc_= "imm"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 4> G_C_CSS_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "imm6"sv, .startBit_= 7, .endBit_= 12, .desc_= "imm"sv },
     { .name_= "rs2"sv, .startBit_= 2, .endBit_= 6, .desc_= "rs2"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 4> G_C_CIW_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "imm8"sv, .startBit_= 5, .endBit_= 12, .desc_= "imm"sv },
     { .name_= "rd'"sv, .startBit_= 2, .endBit_= 4, .desc_= "rd'"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 6> G_C_CL_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "immHi"sv, .startBit_= 10, .endBit_= 12, .desc_= "imm"sv },
     { .name_= "rs1'"sv, .startBit_= 7, .endBit_= 9, .desc_= "rs1'"sv },
     { .name_= "immLo"sv, .startBit_= 5, .endBit_= 6, .desc_= "imm"sv },
     { .name_= "rd'"sv, .startBit_= 2, .endBit_= 4, .desc_= "rd'"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 6> G_C_CS_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "immHi"sv, .startBit_= 10, .endBit_= 12, .desc_= "imm"sv },
     { .name_= "rs1'"sv, .startBit_= 7, .endBit_= 9, .desc_= "rs1'"sv },
     { .name_= "immLo"sv, .startBit_= 5, .endBit_= 6, .desc_= "imm"sv },
     { .name_= "rs2'"sv, .startBit_= 2, .endBit_= 4, .desc_= "rs2'"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 5> G_C_CA_FIELDS {
    {
     { .name_= "funct6"sv, .startBit_= 10, .endBit_= 15, .desc_= "funct6"sv },
     { .name_= "rd'"sv, .startBit_= 7, .endBit_= 9, .desc_= "rd'/rs1'"sv },
     { .name_= "funct2"sv, .startBit_= 5, .endBit_= 6, .desc_= "funct2"sv },
     { .name_= "rs2'"sv, .startBit_= 2, .endBit_= 4, .desc_= "rs2'"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 5> G_C_CB_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "offHi"sv, .startBit_= 10, .endBit_= 12, .desc_= "offset"sv },
     { .name_= "rd'"sv, .startBit_= 7, .endBit_= 9, .desc_= "rd'/rs1'"sv },
     { .name_= "offLo"sv, .startBit_= 2, .endBit_= 6, .desc_= "offset"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

constexpr std::array<InstField, 3> G_C_CJ_FIELDS {
    {
     { .name_= "funct3"sv, .startBit_= 13, .endBit_= 15, .desc_= "funct3"sv },
     { .name_= "target"sv, .startBit_= 2, .endBit_= 12, .desc_= "jump target"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 1, .desc_= "opc"sv },
     }
};

// Indexed by CShape (CR=0 .. CJ=8).
constexpr std::array<std::span<const InstField>, 9> G_C_SHAPE_FIELDS {
    std::span<const InstField>(G_C_CR_FIELDS),
    std::span<const InstField>(G_C_CI_FIELDS),
    std::span<const InstField>(G_C_CSS_FIELDS),
    std::span<const InstField>(G_C_CIW_FIELDS),
    std::span<const InstField>(G_C_CL_FIELDS),
    std::span<const InstField>(G_C_CS_FIELDS),
    std::span<const InstField>(G_C_CA_FIELDS),
    std::span<const InstField>(G_C_CB_FIELDS),
    std::span<const InstField>(G_C_CJ_FIELDS),
};

constexpr std::array<InstField, 6> G_J_FIELDS {
    {
     { .name_= "imm20"sv, .startBit_= 31, .endBit_= 31, .desc_= "imm[20]"sv },
     { .name_= "imm10_1"sv, .startBit_= 21, .endBit_= 30, .desc_= "imm[10:1]"sv },
     { .name_= "imm11"sv, .startBit_= 20, .endBit_= 20, .desc_= "imm[11]"sv },
     { .name_= "imm19_12"sv, .startBit_= 12, .endBit_= 19, .desc_= "imm[19:12]"sv },
     { .name_= "rd"sv, .startBit_= 7, .endBit_= 11, .desc_= "rd"sv },
     { .name_= "opcode"sv, .startBit_= 0, .endBit_= 6, .desc_= "opcode"sv },
     }
};

// Indexed by InstFormat (R=0 .. C=6).
constexpr std::array<std::span<const InstField>, 7> G_FMT_FIELDS {
    std::span<const InstField>(G_R_FIELDS),
    std::span<const InstField>(G_I_FIELDS),
    std::span<const InstField>(G_S_FIELDS),
    std::span<const InstField>(G_B_FIELDS),
    std::span<const InstField>(G_U_FIELDS),
    std::span<const InstField>(G_J_FIELDS),
    std::span<const InstField>(G_C_FIELDS),
};

// Indexed by ItypeImmShape (IMM12=0, SHIFT=1, FENCE=2, CSR=3).
constexpr std::array<std::span<const InstField>, 4> G_I_SHAPE_FIELDS {
    std::span<const InstField>(G_I_FIELDS),
    std::span<const InstField>(G_I_SHIFT_FIELDS),
    std::span<const InstField>(G_I_FENCE_FIELDS),
    std::span<const InstField>(G_I_CSR_FIELDS),
};

} // namespace

InstLayout::InstLayout(uint32_t instruction): entity_(instruction) { }

InstLayout &InstLayout::operator= (uint32_t instruction)
{
    entity_= instruction;
    return *this;
}

InstLayout::operator uint32_t() const
{
    return entity_;
}

std::string_view InstFormatName(InstFormat fmt) noexcept
{
    switch(fmt) {
    case InstFormat::R: return "R-Type";
    case InstFormat::I: return "I-Type";
    case InstFormat::S: return "S-Type";
    case InstFormat::B: return "B-Type";
    case InstFormat::U: return "U-Type";
    case InstFormat::J: return "J-Type";
    case InstFormat::C: return "C-Type";
    default:            return "UNKNOW";
    }
}

std::span<const InstField> GetInstFields(InstFormat fmt)
{
    const auto IDX= static_cast<int>(fmt);
    if(IDX < 0 || std::cmp_greater_equal(IDX, G_FMT_FIELDS.size())) return {};

    return G_FMT_FIELDS[IDX];
}

std::span<const InstField> GetIInstFields(IImmShape shape) noexcept
{
    const auto IDX= static_cast<size_t>(shape);
    if(IDX >= G_I_SHAPE_FIELDS.size()) return {};

    return G_I_SHAPE_FIELDS[IDX];
}

IImmShape DetectIImmShape(uint32_t opc, uint32_t f3) noexcept
{
    // MISC_MEM: f3=0/1 fence*; f3=2 is RV128I LQ (I-imm), not fence layout
    if(OpcMap::MISC_MEM == opc && f3 <= 1) return IImmShape::FENCE;
    // SYSTEM: f3=0 ecall/ebreak (imm12); f3≠0 Zicsr (csr address in [31:20])
    if(OpcMap::SYSTEM == opc && 0 != f3) return IImmShape::CSR;
    if((OpcMap::OP_IMM == opc || OpcMap::OP_IMM_32 == opc || OpcMap::OP_IMM_64 == opc)
       && (1 == f3 || 5 == f3)) {
        return IImmShape::SHIFT;
    }
    return IImmShape::IMM12;
}

std::span<const InstField> GetRInstFields(RShape shape) noexcept
{
    const auto IDX= static_cast<size_t>(shape);
    if(IDX >= G_R_SHAPE_FIELDS.size()) return {};

    return G_R_SHAPE_FIELDS[IDX];
}

RShape DetectRShape(uint32_t opc) noexcept
{
    if(OpcMap::AMO == opc) return RShape::AMO;
    if(OpcMap::OP_FP == opc) return RShape::FP;

    if(OpcMap::MADD == opc
       || OpcMap::MSUB == opc
       || OpcMap::NMSUB == opc
       || OpcMap::NMADD == opc) {
        return RShape::R4;
    }

    return RShape::BASE;
}

std::span<const InstField> GetCInstFields(CShape shape) noexcept
{
    const auto IDX= static_cast<size_t>(shape);
    if(IDX >= G_C_SHAPE_FIELDS.size()) return {};

    return G_C_SHAPE_FIELDS[IDX];
}

CShape DetectCShape(uint32_t inst) noexcept
{
    const uint32_t Q = inst & 0x3;
    const uint32_t F3= (inst >> 13) & 0x7;

    switch(Q) {
    case 0: {
        if(0 == F3) return CShape::CIW;
        if(F3 <= 3) return CShape::CL;
        return CShape::CS;
    }
    case 1:
        switch(F3) {
        case 5: return CShape::CJ;
        case 6:
        case 7: return CShape::CB;
        case 4: {
            const uint32_t MID= (inst >> 10) & 0x7; // inst[12:10]
            return 7 == MID || 3 == MID ? CShape::CA : CShape::CB;
        }
        default: return CShape::CI;
        }
    case 2: {
        if(4 == F3) return CShape::CR;
        if(F3 >= 5) return CShape::CSS;
        return CShape::CI;
    }
    default: return CShape::CI;
    }
}
