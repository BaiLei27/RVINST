#pragma once
#include <array>

#include "IBaseInstType.hh"

class RType: public IBaseInstType {
public:
    // OP/OP_32/OP_64: payload = (funct7 << 3) | funct3
    // AMO:             payload = (funct5 << 3) | funct3  (aq/rl ignored in key)
    constexpr static std::array<InstInfo, 42> G_INST_TABLE= {
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

         { .name_= "addd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 0, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "subd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 0, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "slld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 1, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00 << 3 | 5, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srad", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20 << 3 | 5, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },

         // RV32A (funct3=010)
         { .name_= "lr.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x02 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "sc.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x03 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoswap.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x01 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoadd.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x00 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoxor.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x04 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoand.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x0C << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoor.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x08 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomin.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x10 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomax.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x14 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amominu.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x18 << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomaxu.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x1C << 3 | 2, OpcMap::AMO), .opcode_= OpcMap::AMO },

         // RV64A (funct3=011)
         { .name_= "lr.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x02 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "sc.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x03 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoswap.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x01 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoadd.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x00 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoxor.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x04 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoand.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x0C << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoor.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x08 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomin.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x10 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomax.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x14 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amominu.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x18 << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomaxu.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x1C << 3 | 3, OpcMap::AMO), .opcode_= OpcMap::AMO },
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

    [[nodiscard]] static constexpr KeyT AMO_K(uint8_t f5, uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f5 << 3 | f3, opc); }

    [[nodiscard]] static bool isAmoName(std::string_view name) noexcept;
};
