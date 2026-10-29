#pragma once
#include <array>

#include "IBaseInstType.hh"

class R4Type: public IBaseInstType {
public:
    // R4 fused multiply-add (MADD/MSUB/NMSUB/NMADD): payload = fmt (funct2)
    constexpr static std::array<InstInfo, 12> G_INST_TABLE= {
        {
         { .name_= "fmadd.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01, OpcMap::MADD), .opcode_= OpcMap::MADD },
         { .name_= "fmadd.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03, OpcMap::MADD), .opcode_= OpcMap::MADD },
         { .name_= "fmadd.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00, OpcMap::MADD), .opcode_= OpcMap::MADD },

         { .name_= "fmsub.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01, OpcMap::MSUB), .opcode_= OpcMap::MSUB },
         { .name_= "fmsub.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03, OpcMap::MSUB), .opcode_= OpcMap::MSUB },
         { .name_= "fmsub.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00, OpcMap::MSUB), .opcode_= OpcMap::MSUB },

         { .name_= "fnmsub.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },
         { .name_= "fnmsub.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },
         { .name_= "fnmsub.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },

         { .name_= "fnmadd.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01, OpcMap::NMADD), .opcode_= OpcMap::NMADD },
         { .name_= "fnmadd.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03, OpcMap::NMADD), .opcode_= OpcMap::NMADD },
         { .name_= "fnmadd.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00, OpcMap::NMADD), .opcode_= OpcMap::NMADD },
         }
    };

    R4Type(InstFormat format, uint32_t inst, bool hasSetABI= false);
    R4Type(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;

    [[nodiscard]] static constexpr KeyT R4_K(uint8_t fmt, uint8_t opc) noexcept { return PACK_KEY(fmt, opc); }
};
