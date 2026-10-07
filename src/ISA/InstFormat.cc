#include <array>
#include "ISA/InstFormat.hh"

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

using namespace std::string_view_literals;
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
     { .name_= "imm"sv, .startBit_= 20, .endBit_= 31, .desc_= "imm[11:0]"sv },
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

std::span<const InstField> GetInstFields(InstFormat fmt)
{
    switch(fmt) {
    case InstFormat::R: return G_R_FIELDS;
    case InstFormat::I: return G_I_FIELDS;
    case InstFormat::S: return G_S_FIELDS;
    case InstFormat::B: return G_B_FIELDS;
    case InstFormat::U: return G_U_FIELDS;
    case InstFormat::J: return G_J_FIELDS;
    default:            return {};
    }
}
