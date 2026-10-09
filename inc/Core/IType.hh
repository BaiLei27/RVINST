#pragma once
#include <array>

#include "IBaseInstType.hh"

namespace {

// OpcMap::OP_IMM: table key funct_ is only (imm[11:5] << 3) | funct3 (matches decode calculateFunctKey).
constexpr uint16_t OP_IMM_FK(uint8_t imm7, uint8_t f3)
{
    return (imm7 << 3) | (f3 & 7);
}

// SYSTEM: tag 0x7000 | funct12. Do not use (SYSTEM<<8)|imm — bits[11:8] overlap.
constexpr uint16_t SYSTEM_FK(uint16_t funct12)
{
    return 0x7000U | (funct12 & 0xFFFU);
}

constexpr uint16_t OP_FK(uint8_t opc, uint8_t f3)
{
}
} // namespace

class IType: public IBaseInstType {
public:
    // RV32I / Zifencei: OPIMM = (imm[11:5]<<3)|funct3; SYSTEM = 0x7000|imm12; else (opcode<<8)|funct3.
    constexpr static std::array<InstInfo, 20> G_INST_TABLE= {
        { { .name_= "lb", .XLEN_= "RV32I", .funct_= 0, .opcode_= OpcMap::LOAD },
         { .name_= "lh", .XLEN_= "RV32I", .funct_= 1, .opcode_= OpcMap::LOAD },
         { .name_= "lw", .XLEN_= "RV32I", .funct_= 2, .opcode_= OpcMap::LOAD },
         { .name_= "ld", .XLEN_= "RV32I", .funct_= 3, .opcode_= OpcMap::LOAD },
         { .name_= "lbu", .XLEN_= "RV32I", .funct_= 4, .opcode_= OpcMap::LOAD },
         { .name_= "lhu", .XLEN_= "RV32I", .funct_= 5, .opcode_= OpcMap::LOAD },

         { .name_= "jalr", .XLEN_= "RV32I", .funct_= 0, .opcode_= JALR },

         { .name_= "addi", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 0), .opcode_= OpcMap::OP_IMM },
         { .name_= "slli", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 1), .opcode_= OpcMap::OP_IMM },
         { .name_= "slti", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 2), .opcode_= OpcMap::OP_IMM },
         { .name_= "sltiu", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 3), .opcode_= OpcMap::OP_IMM },
         { .name_= "xori", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 4), .opcode_= OpcMap::OP_IMM },
         { .name_= "srli", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 5), .opcode_= OpcMap::OP_IMM },
         { .name_= "srai", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0x20, 5), .opcode_= OpcMap::OP_IMM },
         { .name_= "ori", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 6), .opcode_= OpcMap::OP_IMM },
         { .name_= "andi", .XLEN_= "RV32I", .funct_= OP_IMM_FK(0, 7), .opcode_= OpcMap::OP_IMM },

         { .name_= "fence", .XLEN_= "RV32I", .funct_= 0, .opcode_= OpcMap::MISC_MEM },
         { .name_= "fence.i", .XLEN_= "Zifencei", .funct_= 1, .opcode_= OpcMap::MISC_MEM },

         { .name_= "ecall", .XLEN_= "RV32I", .funct_= SYSTEM_FK(0), .opcode_= OpcMap::SYSTEM },
         { .name_= "ebreak", .XLEN_= "RV32I", .funct_= SYSTEM_FK(1), .opcode_= OpcMap::SYSTEM } }
    };

public:
    IType(uint32_t inst, InstFormat format, bool hasSetABI= false);
    IType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI= false);

public:
    void Parse() override;
    [[nodiscard]] const InstLayout &Assembly() override;

private:
    KeyT calculateFunctKey() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;
};
