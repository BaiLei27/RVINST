#pragma once
#include <array>

#include "IBaseInstType.hh"

class SType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 5> G_INST_TABLE= {
        {
         { .name_= "sb", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sh", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x01, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sw", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x02, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sd", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x03, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sq", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x04, OpcMap::STORE), .opcode_= OpcMap::STORE },
         }
    };

    SType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    SType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f3, opc); }
};
