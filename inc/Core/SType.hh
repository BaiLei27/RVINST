#pragma once
#include <array>

#include "IBaseInstType.hh"

class SType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 4> G_INST_TABLE= {
        {
         { .name_= "sb", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sh", .XLEN_= "RV32I", .keyID_= PACK_KEY(1, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sw", .XLEN_= "RV32I", .keyID_= PACK_KEY(2, OpcMap::STORE), .opcode_= OpcMap::STORE },
         { .name_= "sd", .XLEN_= "RV64I", .keyID_= PACK_KEY(3, OpcMap::STORE), .opcode_= OpcMap::STORE },
         }
    };

    SType(uint32_t inst, InstFormat format, bool hasSetABI= false);
    SType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] const InstLayout &Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(f3, opc); }
};
