#pragma once

#include <cstdint>
#include <span>
#include <string_view>

enum class InstFormat : int8_t { // Instruction format enumeration
    UNKNOWN= -1,
    R, // Register-Register
    I, // Immediate-Register
    S, // Store instruction
    B, // Branch instruction
    U, // Upper immediate (lui/auipc)
    J, // Jump instruction (jal)
};

enum OpcMap : uint8_t { // RISC-V base opcode map, inst[1:0] === 0b11
    // inst[6:5] = 00
    LOAD     = 0x03, // 00_000_11
    LOAD_FP  = 0x07, // 00_001_11
    CUSTOM_0 = 0x0B, // 00_010_11
    MISC_MEM = 0x0F, // 00_011_11
    OP_IMM   = 0x13, // 00_100_11
    AUIPC    = 0x17, // 00_101_11
    OP_IMM_32= 0x1B, // 00_110_11 (RV64 only)
    RESERVE_1= 0x1F, // 00_111_11

    // inst[6:5] = 01
    STORE    = 0x23, // 01_000_11
    STORE_FP = 0x27, // 01_001_11
    CUSTOM_1 = 0x2B, // 01_010_11
    AMO      = 0x2F, // 01_011_11
    OP       = 0x33, // 01_100_11
    LUI      = 0x37, // 01_101_11
    OP_32    = 0x3B, // 01_110_11 (RV64 only)
    RESERVE_2= 0x3F, // 01_111_11

    // inst[6:5] = 10
    MADD     = 0x43, // 10_000_11
    MSUB     = 0x47, // 10_001_11
    NMSUB    = 0x4B, // 10_010_11
    NMADD    = 0x4F, // 10_011_11
    OP_FP    = 0x53, // 10_100_11
    OP_V     = 0x57, // 10_101_11
    OP_IMM_64= 0x5B, // 10_110_11 (RV128I draft; was custom-2)
    RESERVE_3= 0x5F, // 10_111_11

    // inst[6:5] = 11
    BRANCH   = 0x63, // 11_000_11
    JALR     = 0x67, // 11_001_11
    RESERVE_4= 0x6B, // 11_010_11
    JAL      = 0x6F, // 11_011_11
    SYSTEM   = 0x73, // 11_100_11
    OP_VE    = 0x77, // 11_101_11
    OP_64    = 0x7B, // 11_110_11 (RV128I draft; was custom-3)
    RESERVE_5= 0x7F, // 11_111_11
};
enum class ItypeImmShape : uint8_t {
    IMM12,
    SHIFT,
    FENCE
};

// NOLINTBEGIN

// 32-bit instruction parsing union
union InstLayout {
    uint32_t entity_; // Complete 32-bit instruction

    struct { // R(opcode:[0:6] | rd:[7:11] | funct3:[12:14] | rs1:[15:19] | rs2:[20:24] | funct7:[25:31])
        uint32_t opc : 7;
        uint32_t rd  : 5;
        uint32_t fct3: 3;
        uint32_t rs1 : 5;
        uint32_t rs2 : 5;
        uint32_t fct7: 7;
    } R;

    struct { // I(opcode:[0:6] | rd:[7:11] | funct3:[12:14] | rs1:[15:19] | imm0tB:[20:31])
        uint32_t opc   : 7;
        uint32_t rd    : 5;
        uint32_t fct3  : 3;
        uint32_t rs1   : 5;
        uint32_t imm0tB: 12; // imm0~imm11
    } I;

    struct { // S(opcode:[0:6] | imm0t4:[7:11] | funct3:[12:14] | rs1:[15:19] | rs2:[20:24] | imm5tB:[25:31])
        uint32_t opc   : 7;
        uint32_t imm0t4: 5; // imm0~imm4
        uint32_t fct3  : 3;
        uint32_t rs1   : 5;
        uint32_t rs2   : 5;
        uint32_t imm5tB: 7; // imm5~imm11
    } S;

    struct { // B(opcode:[0:6] | immB:[7:7] | imm1t4:[8:11] | funct3:[12:14] | rs1:[15:19] | rs2:[20:24] | imm5tA:[25:30] | immC:[31:31])
        uint32_t opc   : 7;
        uint32_t immB  : 1; // imm11
        uint32_t imm1t4: 4; // imm1~imm4
        uint32_t fct3  : 3;
        uint32_t rs1   : 5;
        uint32_t rs2   : 5;
        uint32_t imm5tA: 6; // imm5~imm10
        uint32_t immC  : 1; // imm12
    } B;

    struct { // U(opcode:[0:6] | rd:[7:11] | immCt1F:[12:31])
        uint32_t opc    : 7;
        uint32_t rd     : 5;
        uint32_t immCt1F: 20; // imm12~imm31
    } U;

    struct { // J(opcode:[0:6] | rd:[7:11] | immCt13:[12:19] | immB:[20:20] | imm1tA:[21:30] | imm14:[31:31])
        uint32_t opc    : 7;
        uint32_t rd     : 5;
        uint32_t immCt13: 8;  // imm12~imm19
        uint32_t immB   : 1;  // imm11
        uint32_t imm1tA : 10; // imm1~imm10
        uint32_t imm14  : 1;  // imm20
    } J;

    // Constructor: Initialize from 32-bit instruction
    InstLayout(uint32_t instruction= 0);

    InstLayout &operator= (uint32_t instruction);

    operator uint32_t() const;
};

// NOLINTEND

struct InstField {
    std::string_view name_;
    int startBit_ {};
    int endBit_ {};
    std::string_view desc_;
};

[[nodiscard]] std::string_view InstFormatName(InstFormat fmt) noexcept;
[[nodiscard]] std::span<const InstField> GetInstFields(InstFormat fmt);
[[nodiscard]] ItypeImmShape DetectIImmShape(uint32_t opc, uint32_t f3) noexcept;
[[nodiscard]] std::span<const InstField> GetIInstFields(ItypeImmShape shape) noexcept;

// Date:25/12/21/22:34
