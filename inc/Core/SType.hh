#pragma once
#include <array>

#include "IBaseInstType.hh"

namespace STypeKey {
// STORE (opcode 0x23): lookup key (opcode<<8)|funct3 (same pattern as IType non-OP-IMM rows).
constexpr uint16_t OP_FK(uint8_t f3)
{
    return (0x23U << 8) | (f3 & 7U);
}
} // namespace STypeKey

class SType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 4> G_INST_TABLE= {
        { { .name_= "sb", .XLEN_= "RV32I", .funct_= STypeKey::OP_FK(0), .opcode_= 0x23 },
         { .name_= "sh", .XLEN_= "RV32I", .funct_= STypeKey::OP_FK(1), .opcode_= 0x23 },
         { .name_= "sw", .XLEN_= "RV32I", .funct_= STypeKey::OP_FK(2), .opcode_= 0x23 },
         { .name_= "sd", .XLEN_= "RV64I", .funct_= STypeKey::OP_FK(3), .opcode_= 0x23 } }
    };

public:
    SType(uint32_t inst, InstFormat format, bool hasSetABI= false);
    SType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI= false);

public:
    void Parse() override;
    [[nodiscard]] const InstLayout &Assembly() override;

private:
    KeyT calculateFunctKey() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;
};
