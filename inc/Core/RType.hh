#pragma once
#include <array>

#include "IBaseInstType.hh"

class RType: public IBaseInstType {
public:
    // payload = (funct7 << 3) | funct3
    constexpr static std::array<InstInfo, 20> G_INST_TABLE= {
        {
         { .name_= "add", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sub", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20 << 3 | 0, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sll", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 1, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "slt", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 2, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sltu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 3, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "xor", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 4, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "srl", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 5, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sra", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20 << 3 | 5, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "or", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 6, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "and", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 7, OpcMap::OP), .opcode_= OpcMap::OP },

         { .name_= "addw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 0, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "subw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20 << 3 | 0, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sllw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 1, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "srlw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 5, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sraw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20 << 3 | 5, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },

         // RV128I OP-64 (*D on low 64 bits)
         { .name_= "addd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 0, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "subd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 0, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "slld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 1, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 5, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srad", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 5, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         }
    };

    RType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    RType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f7, uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f7 << 3 | f3, opc); }
};
