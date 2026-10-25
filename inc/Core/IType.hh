#pragma once
#include <array>

#include "IBaseInstType.hh"

class IType: public IBaseInstType {
public:
    // OP_IMM* shift: payload = (immFunct << 3) | f3, immFunct is 0 or 0x20 (srai bit)
    // MISC_MEM: fence/fence.i use f3; fence.tso/pause use full imm12; lq uses f3=2
    constexpr static std::array<InstInfo, 35> G_INST_TABLE= {
        {
         { .name_= "lb", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lbu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x04, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "ld", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x03, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "ldu", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x07, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lh", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x01, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lhu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x05, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lw", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x02, OpcMap::LOAD), .opcode_= OpcMap::LOAD },
         { .name_= "lwu", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x06, OpcMap::LOAD), .opcode_= OpcMap::LOAD },

         { .name_= "fld", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x03, OpcMap::LOAD_FP), .opcode_= OpcMap::LOAD_FP },
         { .name_= "flw", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x02, OpcMap::LOAD_FP), .opcode_= OpcMap::LOAD_FP },

         { .name_= "fence", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },
         { .name_= "fence.i", .XLEN_= "Zifencei", .keyID_= PACK_KEY(0x01, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },
         { .name_= "fence.tso", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x833, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },
         { .name_= "lq", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x02, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },
         { .name_= "pause", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x10, OpcMap::MISC_MEM), .opcode_= OpcMap::MISC_MEM },

         { .name_= "addi", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "andi", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x07, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "ori", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x06, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "slli", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "slti", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x02, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "sltiu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x03, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "srai", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "srli", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },
         { .name_= "xori", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x04, OpcMap::OP_IMM), .opcode_= OpcMap::OP_IMM },

         { .name_= "addiw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00, OpcMap::OP_IMM_32), .opcode_= OpcMap::OP_IMM_32 },
         { .name_= "slliw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP_IMM_32), .opcode_= OpcMap::OP_IMM_32 },
         { .name_= "sraiw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP_IMM_32), .opcode_= OpcMap::OP_IMM_32 },
         { .name_= "srliw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP_IMM_32), .opcode_= OpcMap::OP_IMM_32 },

         { .name_= "addid", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00, OpcMap::OP_IMM_64), .opcode_= OpcMap::OP_IMM_64 },
         { .name_= "sllid", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP_IMM_64), .opcode_= OpcMap::OP_IMM_64 },
         { .name_= "sraid", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP_IMM_64), .opcode_= OpcMap::OP_IMM_64 },
         { .name_= "srlid", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP_IMM_64), .opcode_= OpcMap::OP_IMM_64 },

         { .name_= "jalr", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::JALR), .opcode_= OpcMap::JALR },

         { .name_= "ebreak", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x01, OpcMap::SYSTEM), .opcode_= OpcMap::SYSTEM },
         { .name_= "ecall", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::SYSTEM), .opcode_= OpcMap::SYSTEM },
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
