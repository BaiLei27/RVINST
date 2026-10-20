#pragma once
#include <array>

#include "IBaseInstType.hh"

class UType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 2> G_INST_TABLE= {
        {
         { .name_= "auipc", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::AUIPC), .opcode_= OpcMap::AUIPC },
         { .name_= "lui", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00, OpcMap::LUI), .opcode_= OpcMap::LUI },
         }
    };

    UType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    UType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t opc) noexcept { return PACK_KEY(0x00, opc); }
};
