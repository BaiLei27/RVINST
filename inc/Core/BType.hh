#pragma once
#include <array>

#include "IBaseInstType.hh"

namespace BTypeKey {
// BRANCH (opcode 0x63): lookup key (opcode<<8)|funct3.
constexpr uint16_t OPC_F3(uint8_t f3)
{
    return (0x63U << 8) | (f3 & 7U);
}
} // namespace BTypeKey

class BType: public IBaseInstType {
public:
    constexpr static std::array<InstInfo, 6> G_INST_TABLE= {
        { { .name_= "beq", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(0), .opcode_= 0x63 },
         { .name_= "bne", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(1), .opcode_= 0x63 },
         { .name_= "blt", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(4), .opcode_= 0x63 },
         { .name_= "bge", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(5), .opcode_= 0x63 },
         { .name_= "bltu", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(6), .opcode_= 0x63 },
         { .name_= "bgeu", .XLEN_= "RV32I", .funct_= BTypeKey::OPC_F3(7), .opcode_= 0x63 } }
    };

public:
    BType(uint32_t inst, InstFormat format, bool hasSetABI= false);
    BType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI= false);

public:
    void Parse() override;

    [[nodiscard]] const std::vector<std::string> &Disassembly() override;
    [[nodiscard]] const InstLayout &Assembly() override;

private:
    KeyT calculateFunctKey() override;
    void mnemonicHelper() override;
    [[nodiscard]] pBiTable_u buildTable() override;
};
