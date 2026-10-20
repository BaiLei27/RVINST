#include <array>

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

// Indexed by InstFormat (R=0 .. J=5).
constexpr std::array<std::span<const InstField>, 6> G_FMT_FIELDS {
    std::span<const InstField>(G_R_FIELDS),
    std::span<const InstField>(G_I_FIELDS),
    std::span<const InstField>(G_S_FIELDS),
    std::span<const InstField>(G_B_FIELDS),
    std::span<const InstField>(G_U_FIELDS),
    std::span<const InstField>(G_J_FIELDS),
};

// Indexed by ItypeImmShape (IMM12=0, SHIFT=1, FENCE=2).
constexpr std::array<std::span<const InstField>, 3> G_I_SHAPE_FIELDS {
    std::span<const InstField>(G_I_FIELDS),
    std::span<const InstField>(G_I_SHIFT_FIELDS),
    std::span<const InstField>(G_I_FENCE_FIELDS),
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
    default:            return "UNKNOW";
    }
}

std::span<const InstField> GetInstFields(InstFormat fmt)
{
    const auto IDX= static_cast<int>(fmt);
    if(IDX < 0 || IDX >= static_cast<int>(G_FMT_FIELDS.size())) return {};

    return G_FMT_FIELDS[IDX];
}

std::span<const InstField> GetIInstFields(ItypeImmShape shape) noexcept
{
    const auto IDX= static_cast<size_t>(shape);
    if(IDX >= G_I_SHAPE_FIELDS.size()) return {};

    return G_I_SHAPE_FIELDS[IDX];
}

ItypeImmShape DetectIImmShape(uint32_t opc, uint32_t f3) noexcept
{
    // MISC_MEM: f3=0/1 fence*; f3=2 is RV128I LQ (I-imm), not fence layout
    if(OpcMap::MISC_MEM == opc && f3 <= 1) return ItypeImmShape::FENCE;
    if((OpcMap::OP_IMM == opc || OpcMap::OP_IMM_32 == opc || OpcMap::OP_IMM_64 == opc)
       && (1 == f3 || 5 == f3)) {
        return ItypeImmShape::SHIFT;
    }
    return ItypeImmShape::IMM12;
}
