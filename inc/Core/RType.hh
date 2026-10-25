#pragma once
#include <array>

#include "IBaseInstType.hh"

class RType: public IBaseInstType {
public:
    // OP/OP_32/OP_64: payload = (funct7 << 3) | funct3  (M uses funct7=0x01)
    // AMO:             payload = (funct5 << 3) | funct3  (aq/rl ignored in key)
    constexpr static std::array<InstInfo, 55> G_INST_TABLE= {
        {
         { .name_= "add", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "and", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x07, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "div", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x04, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "divu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mul", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulh", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x01, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulhsu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x02, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulhu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x03, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "or", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x06, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "rem", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x06, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "remu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01 << 3 | 0x07, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sll", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x01, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "slt", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x02, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sltu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x03, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sra", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20 << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "srl", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sub", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20 << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "xor", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00 << 3 | 0x04, OpcMap::OP), .opcode_= OpcMap::OP },

         { .name_= "addw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "divuw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01 << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "divw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01 << 3 | 0x04, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "mulw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01 << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "remuw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01 << 3 | 0x07, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "remw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01 << 3 | 0x06, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sllw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 0x01, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sraw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20 << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "srlw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00 << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "subw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20 << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },

         { .name_= "addd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 0x00, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "slld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 0x01, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srad", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 0x05, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 0x05, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "subd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 0x00, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
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
