#pragma once
#include <array>

#include "IBaseInstType.hh"

class RType: public IBaseInstType {
public:
    // OP/OP_32/OP_64: payload = (funct7 << 3) | funct3  (M uses funct7=0x01)
    // AMO:             payload = (funct5 << 3) | funct3  (aq/rl ignored in key)
    // R4 (fmadd*):     payload = fmt (funct2)
    // OP_FP:           payload = (((funct5 << 2) | fmt) << 8) | (rs2 << 3) | f3key
    //                  (rm ops keep f3key=0; rs2/f3key slots use 0U<<3|0 when unused)
    constexpr static std::array<InstInfo, 143> G_INST_TABLE= {
        {
         { .name_= "amoadd.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x00U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoadd.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x00U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoand.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x0CU << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoand.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x0CU << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomax.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x14U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomax.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x14U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomaxu.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x1CU << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomaxu.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x1CU << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomin.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x10U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amomin.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x10U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amominu.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x18U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amominu.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x18U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoor.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x08U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoor.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x08U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoswap.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x01U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoswap.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x01U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoxor.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x04U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "amoxor.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x04U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "lr.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x02U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "lr.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x02U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "sc.d", .XLEN_= "RV64A", .keyID_= PACK_KEY(0x03U << 3 | 0x03, OpcMap::AMO), .opcode_= OpcMap::AMO },
         { .name_= "sc.w", .XLEN_= "RV32A", .keyID_= PACK_KEY(0x03U << 3 | 0x02, OpcMap::AMO), .opcode_= OpcMap::AMO },

         { .name_= "add", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "and", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x07, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "div", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x04, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "divu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mul", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulh", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x01, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulhsu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x02, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "mulhu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x03, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "or", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x06, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "rem", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x06, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "remu", .XLEN_= "RV32M", .keyID_= PACK_KEY(0x01U << 3 | 0x07, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sll", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "slt", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x02, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sltu", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x03, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sra", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "srl", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "sub", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x20U << 3 | 0x00, OpcMap::OP), .opcode_= OpcMap::OP },
         { .name_= "xor", .XLEN_= "RV32I", .keyID_= PACK_KEY(0x00U << 3 | 0x04, OpcMap::OP), .opcode_= OpcMap::OP },

         { .name_= "addw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00U << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "divuw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01U << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "divw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01U << 3 | 0x04, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "mulw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01U << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "remuw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01U << 3 | 0x07, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "remw", .XLEN_= "RV64M", .keyID_= PACK_KEY(0x01U << 3 | 0x06, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sllw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "sraw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "srlw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },
         { .name_= "subw", .XLEN_= "RV64I", .keyID_= PACK_KEY(0x20U << 3 | 0x00, OpcMap::OP_32), .opcode_= OpcMap::OP_32 },

         { .name_= "fmadd.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01U, OpcMap::MADD), .opcode_= OpcMap::MADD },
         { .name_= "fmadd.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03U, OpcMap::MADD), .opcode_= OpcMap::MADD },
         { .name_= "fmadd.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00U, OpcMap::MADD), .opcode_= OpcMap::MADD },

         { .name_= "fmsub.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01U, OpcMap::MSUB), .opcode_= OpcMap::MSUB },
         { .name_= "fmsub.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03U, OpcMap::MSUB), .opcode_= OpcMap::MSUB },
         { .name_= "fmsub.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00U, OpcMap::MSUB), .opcode_= OpcMap::MSUB },

         { .name_= "fnmsub.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01U, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },
         { .name_= "fnmsub.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03U, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },
         { .name_= "fnmsub.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00U, OpcMap::NMSUB), .opcode_= OpcMap::NMSUB },

         { .name_= "fnmadd.d", .XLEN_= "RV32D", .keyID_= PACK_KEY(0x01U, OpcMap::NMADD), .opcode_= OpcMap::NMADD },
         { .name_= "fnmadd.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY(0x03U, OpcMap::NMADD), .opcode_= OpcMap::NMADD },
         { .name_= "fnmadd.s", .XLEN_= "RV32F", .keyID_= PACK_KEY(0x00U, OpcMap::NMADD), .opcode_= OpcMap::NMADD },

         { .name_= "fadd.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x00U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fadd.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x00U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fadd.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x00U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fclass.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x1CU << 2 | 0x01) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fclass.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x1CU << 2 | 0x03) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fclass.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x1CU << 2 | 0x00) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.l", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x1AU << 2 | 0x01) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.lu", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x1AU << 2 | 0x01) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x08U << 2 | 0x01) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.s", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x08U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.w", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x1AU << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.d.wu", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x1AU << 2 | 0x01) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.l.d", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x18U << 2 | 0x01) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.l.q", .XLEN_= "RV64Q", .keyID_= PACK_KEY((0x18U << 2 | 0x03) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.l.s", .XLEN_= "RV64F", .keyID_= PACK_KEY((0x18U << 2 | 0x00) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.lu.d", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x18U << 2 | 0x01) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.lu.q", .XLEN_= "RV64Q", .keyID_= PACK_KEY((0x18U << 2 | 0x03) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.lu.s", .XLEN_= "RV64F", .keyID_= PACK_KEY((0x18U << 2 | 0x00) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.d", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x08U << 2 | 0x03) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.l", .XLEN_= "RV64Q", .keyID_= PACK_KEY((0x1AU << 2 | 0x03) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.lu", .XLEN_= "RV64Q", .keyID_= PACK_KEY((0x1AU << 2 | 0x03) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.s", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x08U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.w", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x1AU << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.q.wu", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x1AU << 2 | 0x03) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x08U << 2 | 0x00) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.l", .XLEN_= "RV64F", .keyID_= PACK_KEY((0x1AU << 2 | 0x00) << 8 | 0x02U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.lu", .XLEN_= "RV64F", .keyID_= PACK_KEY((0x1AU << 2 | 0x00) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x08U << 2 | 0x00) << 8 | 0x03U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.w", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x1AU << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.s.wu", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x1AU << 2 | 0x00) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.w.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x18U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.w.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x18U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.w.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x18U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.wu.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x18U << 2 | 0x01) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.wu.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x18U << 2 | 0x03) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fcvt.wu.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x18U << 2 | 0x00) << 8 | 0x01U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fdiv.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x03U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fdiv.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x03U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fdiv.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x03U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "feq.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x14U << 2 | 0x01) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "feq.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x14U << 2 | 0x03) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "feq.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x14U << 2 | 0x00) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fle.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x14U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fle.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x14U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fle.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x14U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "flt.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x14U << 2 | 0x01) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "flt.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x14U << 2 | 0x03) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "flt.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x14U << 2 | 0x00) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmax.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x05U << 2 | 0x01) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmax.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x05U << 2 | 0x03) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmax.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x05U << 2 | 0x00) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmin.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x05U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmin.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x05U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmin.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x05U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmul.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x02U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmul.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x02U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmul.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x02U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmv.d.x", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x1EU << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmv.w.x", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x1EU << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmv.x.d", .XLEN_= "RV64D", .keyID_= PACK_KEY((0x1CU << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fmv.x.w", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x1CU << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnj.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x04U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnj.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x04U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnj.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x04U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjn.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x04U << 2 | 0x01) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjn.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x04U << 2 | 0x03) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjn.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x04U << 2 | 0x00) << 8 | 0x00U << 3 | 0x01, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjx.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x04U << 2 | 0x01) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjx.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x04U << 2 | 0x03) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsgnjx.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x04U << 2 | 0x00) << 8 | 0x00U << 3 | 0x02, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsqrt.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x0BU << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsqrt.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x0BU << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsqrt.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x0BU << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsub.d", .XLEN_= "RV32D", .keyID_= PACK_KEY((0x01U << 2 | 0x01) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsub.q", .XLEN_= "RV32Q", .keyID_= PACK_KEY((0x01U << 2 | 0x03) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },
         { .name_= "fsub.s", .XLEN_= "RV32F", .keyID_= PACK_KEY((0x01U << 2 | 0x00) << 8 | 0x00U << 3 | 0x00, OpcMap::OP_FP), .opcode_= OpcMap::OP_FP },

         { .name_= "addd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00U << 3 | 0x00, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "slld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00U << 3 | 0x01, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srad", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20U << 3 | 0x05, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "srld", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x00U << 3 | 0x05, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
         { .name_= "subd", .XLEN_= "RV128I", .keyID_= PACK_KEY(0x20U << 3 | 0x00, OpcMap::OP_64), .opcode_= OpcMap::OP_64 },
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

    [[nodiscard]] static constexpr KeyT OP_K(uint8_t f7, uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(static_cast<KeyT>(f7) << 3 | f3, opc); }

    [[nodiscard]] static constexpr KeyT AMO_K(uint8_t f5, uint8_t f3, uint8_t opc) noexcept { return PACK_KEY(static_cast<KeyT>(f5) << 3 | f3, opc); }

    [[nodiscard]] static constexpr KeyT R4_K(uint8_t fmt, uint8_t opc) noexcept { return PACK_KEY(fmt, opc); }

    [[nodiscard]] static constexpr KeyT FP_K(uint8_t f7, uint8_t rs2, uint8_t f3key) noexcept
    {
        return PACK_KEY(static_cast<KeyT>(f7) << 8 | static_cast<KeyT>(rs2 & 0x1F) << 3 | (f3key & 0x07), OpcMap::OP_FP);
    }

    [[nodiscard]] static bool isAmoName(std::string_view name) noexcept;
    [[nodiscard]] static bool isR4Opc(uint8_t opc) noexcept;
    [[nodiscard]] static bool fpUsesRm(uint8_t f7) noexcept;
};
