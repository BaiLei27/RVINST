#pragma once

#include <array>

#include "IBaseInstType.hh"

class CType: public IBaseInstType {
public:
    // RVC key payload (before PACK_KEY): (funct3 << 8) | (subfunct << 2) | quadrant
    // funct3 = inst[15:13]; quadrant = inst[1:0] (0,1,2) = .opcode_
    // subfunct holds non-operand distinguishing bits ([12], [11:10], [6:5], etc.) per opcode group
    constexpr static std::array<InstInfo, 42> G_INST_TABLE= {
        {
         { .name_= "c.addi4spn", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x00U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.fld", .XLEN_= "RV32DC", .keyID_= PACK_KEY(0x01U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.flw", .XLEN_= "RV32FC", .keyID_= PACK_KEY(0x03U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.fsd", .XLEN_= "RV32DC", .keyID_= PACK_KEY(0x05U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.fsw", .XLEN_= "RV32FC", .keyID_= PACK_KEY(0x07U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.ld", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x03U << 8 | 0x01U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.lw", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x02U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.sd", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x07U << 8 | 0x01U << 2 | 0x00, 0x00), .opcode_= 0x00 },
         { .name_= "c.sw", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x06U << 8 | 0x00U << 2 | 0x00, 0x00), .opcode_= 0x00 },

         { .name_= "c.addi", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x00U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.addi16sp", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x03U << 8 | 0x01U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.addiw", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x01U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.addw", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x04U << 8 | 0x0DU << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.and", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x1FU << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.andi", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x08U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.beqz", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x06U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.bnez", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x07U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.j", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x05U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.jal", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x01U << 8 | 0x01U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.li", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x02U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.lui", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x03U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.nop", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x00U << 8 | 0x01U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.or", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x1EU << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.srai", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x01U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.srli", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x00U << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.sub", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x1CU << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.subw", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x04U << 8 | 0x0CU << 2 | 0x01, 0x01), .opcode_= 0x01 },
         { .name_= "c.xor", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x1DU << 2 | 0x01, 0x01), .opcode_= 0x01 },

         { .name_= "c.add", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x03U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.ebreak", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x02U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.fldsp", .XLEN_= "RV32DC", .keyID_= PACK_KEY(0x01U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.flwsp", .XLEN_= "RV32FC", .keyID_= PACK_KEY(0x03U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.fsdsp", .XLEN_= "RV32DC", .keyID_= PACK_KEY(0x05U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.fswsp", .XLEN_= "RV32FC", .keyID_= PACK_KEY(0x07U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.jalr", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x01U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.jr", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.ldsp", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x03U << 8 | 0x01U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.lwsp", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x02U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.mv", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x04U << 8 | 0x04U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.sdsp", .XLEN_= "RV64C", .keyID_= PACK_KEY(0x07U << 8 | 0x01U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.slli", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x00U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         { .name_= "c.swsp", .XLEN_= "RV32C", .keyID_= PACK_KEY(0x06U << 8 | 0x00U << 2 | 0x02, 0x02), .opcode_= 0x02 },
         }
    };

    CType(InstFormat format, uint32_t inst, bool hasSetABI= false);
    CType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI= false);

    void Parse() override;
    [[nodiscard]] util::Expected_u<const InstLayout *> Assembly() override;

private:
    KeyT calculateKeyID() override;
    void mnemonicHelper() override;
    [[nodiscard]] const BiLookupTable<KeyT> *buildTable() override;
};
