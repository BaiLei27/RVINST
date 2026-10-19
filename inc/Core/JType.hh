#pragma once
#include <array>

#include "IBaseInstType.hh"

class JType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 1> G_INST_TABLE= { { { .name_= "jal", .XLEN_= "RV32I", .keyID_= PACK_KEY(0, OpcMap::JAL), .opcode_= OpcMap::JAL } } };

    JType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    JType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    [[nodiscard]] static constexpr KeyT OP_K(uint8_t opc) noexcept { return PACK_KEY(0, opc); }

    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;
};
