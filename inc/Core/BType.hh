#pragma once
#include <array>

#include "IBaseInstType.hh"

class BType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 6> G_INST_TABLE= {
        {
         { .name_= "beq", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         { .name_= "bne", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x01, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         { .name_= "blt", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x04, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         { .name_= "bge", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x05, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         { .name_= "bltu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x06, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         { .name_= "bgeu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x07, OpcMap::BRANCH), .opcode_= OpcMap::BRANCH },
         }
    };

    BType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    BType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f3, opc); }
};
