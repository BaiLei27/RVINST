#pragma once
#include <array>

#include "IBaseInstType.hh"

class IType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 20> G_INST_TABLE= {
        {
         { .name_= "lb", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lh", .XLEN_= "RV32I", .keyID_= PACK_KEY(1, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lw", .XLEN_= "RV32I", .keyID_= PACK_KEY(2, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "ld", .XLEN_= "RV32I", .keyID_= PACK_KEY(3, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lbu", .XLEN_= "RV32I", .keyID_= PACK_KEY(4, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lhu", .XLEN_= "RV32I", .keyID_= PACK_KEY(5, OpcMap::LOAD), .opcode_= OpcMap::LOAD },

         { .name_= "jalr", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::JALR), .opcode_= OpcMap::JALR },

         { .name_= "addi", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "slli", .XLEN_= "RV32I", .keyID_= PACK_KEY(1, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "slti", .XLEN_= "RV32I", .keyID_= PACK_KEY(2, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "sltiu", .XLEN_= "RV32I", .keyID_= PACK_KEY(3, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "xori", .XLEN_= "RV32I", .keyID_= PACK_KEY(4, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "srli", .XLEN_= "RV32I", .keyID_= PACK_KEY(5, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "srai", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20 << 3 | 5, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "ori", .XLEN_= "RV32I", .keyID_= PACK_KEY(6, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "andi", .XLEN_= "RV32I", .keyID_= PACK_KEY(7, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },

         { .name_= "fence", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },
         { .name_= "fence.i", .XLEN_= "Zifencei", .keyID_= PACK_KEY(1, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },

         { .name_= "ecall", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::SYSTEM), .opcode_= OpcMap::SYSTEM },
         { .name_= "ebreak", .XLEN_= "RV32I", .keyID_= PACK_KEY(1, OpcMap::SYSTEM), .opcode_= OpcMap::SYSTEM },
         }
    };

    IType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    IType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_IMM_K(uint8_t imm7, uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(imm7 << 3 | f3, opc); }

    [[nodiscard]] static constexpr KeyT SYSTEM_K(uint16_t f12, uint8_t opc) noexcept { return PACK_KEY(f12, opc); }

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f3, opc); }
};
