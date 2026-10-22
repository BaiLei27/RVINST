#include "Util/InstFormatView.hh"

namespace util {
namespace {

    constexpr std::string_view G_R_ASM[] { "mnemonic", "rd", ",", "rs1", ",", "rs2" };
    constexpr std::string_view G_R_R4_ASM[] { "mnemonic", "rd", ",", "rs1", ",", "rs2", ",", "rs3" };
    constexpr std::string_view G_I_ASM[] { "mnemonic", "rd", ",", "rs1", ",", "imm" };
    constexpr std::string_view G_I_CSR_ASM[] { "mnemonic", "rd", ",", "csr", ",", "rs1" };
    constexpr std::string_view G_I_FENCE_ASM[] { "mnemonic", "pred", ",", "succ" };
    constexpr std::string_view G_J_ASM[] { "mnemonic", "rd", ",", "imm" };
    constexpr std::string_view G_U_ASM[] { "mnemonic", "rd", ",", "imm" };
    constexpr std::string_view G_S_ASM[] { "mnemonic", "rs2", ",", "imm", "(", "rs1", ")" };
    constexpr std::string_view G_B_ASM[] { "mnemonic", "rs1", ",", "rs2", ",", "imm" };
    // C operand layouts vary widely; mnemonic chip + 16-bit field diagram is enough for the UI.
    constexpr std::string_view G_C_ASM[] { "mnemonic" };

    constexpr std::string_view G_R_KEY[] { "opcode", "funct3", "funct7" };
    constexpr std::string_view G_R_AMO_KEY[] { "opcode", "funct3", "funct5" };
    constexpr std::string_view G_R_FP_KEY[] { "opcode", "funct5", "fmt", "rm" };
    constexpr std::string_view G_R_R4_KEY[] { "opcode", "fmt", "rm" };
    constexpr std::string_view G_I_KEY[] { "opcode", "funct3" };
    constexpr std::string_view G_I_SHIFT_KEY[] { "opcode", "funct3", "imm7" };
    constexpr std::string_view G_S_KEY[] { "opcode", "funct3" };
    constexpr std::string_view G_U_KEY[] { "opcode" };
    constexpr std::string_view G_J_KEY[] { "opcode" };
    constexpr std::string_view G_C_KEY[] { "quadrant", "inst15_2" };

    constexpr std::string_view G_R_BIN_OPC[] { "mnemonic", "funct3", "funct7" };
    constexpr std::string_view G_R_BIN_F3[] { "mnemonic", "opcode", "funct7" };
    constexpr std::string_view G_R_BIN_F7[] { "mnemonic", "opcode", "funct3" };
    constexpr std::string_view G_SELF_RD[] { "rd" };
    constexpr std::string_view G_SELF_RS1[] { "rs1" };
    constexpr std::string_view G_SELF_RS2[] { "rs2" };
    constexpr std::string_view G_SELF_IMM[] { "imm" };
    constexpr std::string_view G_R_ASM_MNE[] { "opcode", "funct3", "funct7" };

    constexpr std::string_view G_I_BIN_OPC[] { "mnemonic", "funct3" };
    constexpr std::string_view G_I_BIN_F3[] { "mnemonic", "opcode" };
    constexpr std::string_view G_I_BIN_IMM[] { "imm" };
    constexpr std::string_view G_I_ASM_MNE[] { "opcode", "funct3" };
    constexpr std::string_view G_I_ASM_IMM[] { "imm11_0" };
    constexpr std::string_view G_SELF_CSR[] { "csr" };
    constexpr std::string_view G_I_BIN_CSR[] { "csr" };

    constexpr std::string_view G_I_SHIFT_BIN_OPC[] { "mnemonic", "funct3", "imm7" };
    constexpr std::string_view G_I_SHIFT_BIN_F3[] { "mnemonic", "opcode", "imm7" };
    constexpr std::string_view G_I_SHIFT_BIN_IMM7[] { "mnemonic", "opcode", "funct3" };
    constexpr std::string_view G_I_SHIFT_BIN_SHAMT[] { "imm" };
    constexpr std::string_view G_I_SHIFT_ASM_MNE[] { "opcode", "funct3", "imm7" };
    constexpr std::string_view G_I_SHIFT_ASM_IMM[] { "imm7", "shamt" };

    constexpr std::string_view G_SELF_PRED[] { "pred" };
    constexpr std::string_view G_SELF_SUCC[] { "succ" };

    constexpr std::string_view G_MNE_ONLY[] { "mnemonic" };
    constexpr std::string_view G_J_ASM_IMM[] { "imm20", "imm10_1", "imm11", "imm19_12" };
    constexpr std::string_view G_U_ASM_IMM[] { "imm31_12" };

    constexpr std::string_view G_S_BIN_OPC[] { "mnemonic", "funct3" };
    constexpr std::string_view G_S_BIN_F3[] { "mnemonic", "opcode" };
    constexpr std::string_view G_S_ASM_MNE[] { "opcode", "funct3" };
    constexpr std::string_view G_S_ASM_IMM[] { "imm11_5", "imm4_0" };
    constexpr std::string_view G_S_BIN_IMM11[] { "imm", "imm4_0" };
    constexpr std::string_view G_S_BIN_IMM4[] { "imm", "imm11_5" };
    constexpr std::string_view G_B_ASM_IMM[] { "imm12", "imm10_5", "imm4_1", "imm11" };
    constexpr std::string_view G_B_BIN_IMM12[] { "imm", "imm10_5", "imm4_1", "imm11" };
    constexpr std::string_view G_B_BIN_IMM10[] { "imm", "imm12", "imm4_1", "imm11" };
    constexpr std::string_view G_B_BIN_IMM4[] { "imm", "imm12", "imm10_5", "imm11" };
    constexpr std::string_view G_B_BIN_IMM11[] { "imm", "imm12", "imm10_5", "imm4_1" };
    constexpr std::string_view G_J_BIN_IMM20[] { "imm", "imm10_1", "imm11", "imm19_12" };
    constexpr std::string_view G_J_BIN_IMM10[] { "imm", "imm20", "imm11", "imm19_12" };
    constexpr std::string_view G_J_BIN_IMM11[] { "imm", "imm20", "imm10_1", "imm19_12" };
    constexpr std::string_view G_J_BIN_IMM19[] { "imm", "imm20", "imm10_1", "imm11" };

    constexpr FieldRel G_R_BIN_REL[] {
        { .key_= "opcode", .related_= G_R_BIN_OPC },
        { .key_= "funct3", .related_= G_R_BIN_F3  },
        { .key_= "funct7", .related_= G_R_BIN_F7  },
        { .key_= "rd",     .related_= G_SELF_RD   },
        { .key_= "rs1",    .related_= G_SELF_RS1  },
        { .key_= "rs2",    .related_= G_SELF_RS2  },
    };
    constexpr FieldRel G_R_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_R_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD   },
        { .key_= "rs1",      .related_= G_SELF_RS1  },
        { .key_= "rs2",      .related_= G_SELF_RS2  },
    };

    constexpr std::string_view G_R_AMO_BIN_OPC[] { "mnemonic", "funct3", "funct5" };
    constexpr std::string_view G_R_AMO_BIN_F3[] { "mnemonic", "opcode", "funct5" };
    constexpr std::string_view G_R_AMO_BIN_F5[] { "mnemonic", "opcode", "funct3" };
    constexpr std::string_view G_R_AMO_ASM_MNE[] { "opcode", "funct3", "funct5" };
    constexpr std::string_view G_SELF_AQ[] { "aq" };
    constexpr std::string_view G_SELF_RL[] { "rl" };

    constexpr FieldRel G_R_AMO_BIN_REL[] {
        { .key_= "opcode", .related_= G_R_AMO_BIN_OPC },
        { .key_= "funct3", .related_= G_R_AMO_BIN_F3  },
        { .key_= "funct5", .related_= G_R_AMO_BIN_F5  },
        { .key_= "aq",     .related_= G_SELF_AQ       },
        { .key_= "rl",     .related_= G_SELF_RL       },
        { .key_= "rd",     .related_= G_SELF_RD       },
        { .key_= "rs1",    .related_= G_SELF_RS1      },
        { .key_= "rs2",    .related_= G_SELF_RS2      },
    };
    constexpr FieldRel G_R_AMO_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_R_AMO_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD       },
        { .key_= "rs1",      .related_= G_SELF_RS1      },
        { .key_= "rs2",      .related_= G_SELF_RS2      },
    };

    constexpr std::string_view G_R_FP_BIN_OPC[] { "mnemonic", "funct5", "fmt" };
    constexpr std::string_view G_R_FP_BIN_F5[] { "mnemonic", "opcode", "fmt" };
    constexpr std::string_view G_R_FP_BIN_FMT[] { "mnemonic", "opcode", "funct5" };
    constexpr std::string_view G_R_FP_ASM_MNE[] { "opcode", "funct5", "fmt" };
    constexpr std::string_view G_SELF_FMT[] { "fmt" };
    constexpr std::string_view G_SELF_RM[] { "rm" };
    constexpr std::string_view G_SELF_RS3[] { "rs3" };

    constexpr FieldRel G_R_FP_BIN_REL[] {
        { .key_= "opcode", .related_= G_R_FP_BIN_OPC },
        { .key_= "funct5", .related_= G_R_FP_BIN_F5  },
        { .key_= "fmt",    .related_= G_R_FP_BIN_FMT },
        { .key_= "rm",     .related_= G_SELF_RM      },
        { .key_= "rd",     .related_= G_SELF_RD      },
        { .key_= "rs1",    .related_= G_SELF_RS1     },
        { .key_= "rs2",    .related_= G_SELF_RS2     },
    };
    constexpr FieldRel G_R_FP_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_R_FP_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD      },
        { .key_= "rs1",      .related_= G_SELF_RS1     },
        { .key_= "rs2",      .related_= G_SELF_RS2     },
    };

    constexpr std::string_view G_R_R4_BIN_OPC[] { "mnemonic", "fmt" };
    constexpr std::string_view G_R_R4_BIN_FMT[] { "mnemonic", "opcode" };
    constexpr std::string_view G_R_R4_ASM_MNE[] { "opcode", "fmt" };

    constexpr FieldRel G_R_R4_BIN_REL[] {
        { .key_= "opcode", .related_= G_R_R4_BIN_OPC },
        { .key_= "fmt",    .related_= G_R_R4_BIN_FMT },
        { .key_= "rm",     .related_= G_SELF_RM      },
        { .key_= "rd",     .related_= G_SELF_RD      },
        { .key_= "rs1",    .related_= G_SELF_RS1     },
        { .key_= "rs2",    .related_= G_SELF_RS2     },
        { .key_= "rs3",    .related_= G_SELF_RS3     },
    };
    constexpr FieldRel G_R_R4_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_R_R4_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD      },
        { .key_= "rs1",      .related_= G_SELF_RS1     },
        { .key_= "rs2",      .related_= G_SELF_RS2     },
        { .key_= "rs3",      .related_= G_SELF_RS3     },
    };

    constexpr FieldRel G_I_BIN_REL[] {
        { .key_= "opcode",  .related_= G_I_BIN_OPC },
        { .key_= "funct3",  .related_= G_I_BIN_F3  },
        { .key_= "imm11_0", .related_= G_I_BIN_IMM },
        { .key_= "rd",      .related_= G_SELF_RD   },
        { .key_= "rs1",     .related_= G_SELF_RS1  },
    };
    constexpr FieldRel G_I_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_I_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD   },
        { .key_= "rs1",      .related_= G_SELF_RS1  },
        { .key_= "imm",      .related_= G_I_ASM_IMM },
    };

    constexpr FieldRel G_I_SHIFT_BIN_REL[] {
        { .key_= "opcode", .related_= G_I_SHIFT_BIN_OPC   },
        { .key_= "funct3", .related_= G_I_SHIFT_BIN_F3    },
        { .key_= "imm7",   .related_= G_I_SHIFT_BIN_IMM7  },
        { .key_= "shamt",  .related_= G_I_SHIFT_BIN_SHAMT },
        { .key_= "rd",     .related_= G_SELF_RD           },
        { .key_= "rs1",    .related_= G_SELF_RS1          },
    };
    constexpr FieldRel G_I_SHIFT_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_I_SHIFT_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD         },
        { .key_= "rs1",      .related_= G_SELF_RS1        },
        { .key_= "imm",      .related_= G_I_SHIFT_ASM_IMM },
    };

    constexpr FieldRel G_I_FENCE_BIN_REL[] {
        { .key_= "opcode", .related_= G_I_BIN_OPC },
        { .key_= "funct3", .related_= G_I_BIN_F3  },
        { .key_= "pred",   .related_= G_SELF_PRED },
        { .key_= "succ",   .related_= G_SELF_SUCC },
        { .key_= "rd",     .related_= G_SELF_RD   },
        { .key_= "rs1",    .related_= G_SELF_RS1  },
    };
    constexpr FieldRel G_I_FENCE_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_I_ASM_MNE },
        { .key_= "pred",     .related_= G_SELF_PRED },
        { .key_= "succ",     .related_= G_SELF_SUCC },
    };

    constexpr FieldRel G_I_CSR_BIN_REL[] {
        { .key_= "opcode", .related_= G_I_BIN_OPC },
        { .key_= "funct3", .related_= G_I_BIN_F3  },
        { .key_= "csr",    .related_= G_I_BIN_CSR },
        { .key_= "rd",     .related_= G_SELF_RD   },
        { .key_= "rs1",    .related_= G_SELF_RS1  },
    };
    constexpr FieldRel G_I_CSR_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_I_ASM_MNE },
        { .key_= "rd",       .related_= G_SELF_RD   },
        { .key_= "csr",      .related_= G_SELF_CSR  },
        { .key_= "rs1",      .related_= G_SELF_RS1  },
    };

    constexpr FieldRel G_J_BIN_REL[] {
        { .key_= "opcode",   .related_= G_MNE_ONLY    },
        { .key_= "imm20",    .related_= G_J_BIN_IMM20 },
        { .key_= "imm10_1",  .related_= G_J_BIN_IMM10 },
        { .key_= "imm11",    .related_= G_J_BIN_IMM11 },
        { .key_= "imm19_12", .related_= G_J_BIN_IMM19 },
        { .key_= "rd",       .related_= G_SELF_RD     },
    };
    constexpr FieldRel G_J_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_MNE_ONLY  },
        { .key_= "rd",       .related_= G_SELF_RD   },
        { .key_= "imm",      .related_= G_J_ASM_IMM },
    };

    constexpr FieldRel G_U_BIN_REL[] {
        { .key_= "opcode",   .related_= G_MNE_ONLY },
        { .key_= "imm31_12", .related_= G_SELF_IMM },
        { .key_= "rd",       .related_= G_SELF_RD  },
    };
    constexpr FieldRel G_U_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_MNE_ONLY  },
        { .key_= "rd",       .related_= G_SELF_RD   },
        { .key_= "imm",      .related_= G_U_ASM_IMM },
    };

    constexpr FieldRel G_S_BIN_REL[] {
        { .key_= "opcode",  .related_= G_S_BIN_OPC   },
        { .key_= "funct3",  .related_= G_S_BIN_F3    },
        { .key_= "imm11_5", .related_= G_S_BIN_IMM11 },
        { .key_= "imm4_0",  .related_= G_S_BIN_IMM4  },
        { .key_= "rs1",     .related_= G_SELF_RS1    },
        { .key_= "rs2",     .related_= G_SELF_RS2    },
    };
    constexpr FieldRel G_S_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_S_ASM_MNE },
        { .key_= "rs2",      .related_= G_SELF_RS2  },
        { .key_= "imm",      .related_= G_S_ASM_IMM },
        { .key_= "rs1",      .related_= G_SELF_RS1  },
    };

    constexpr FieldRel G_B_BIN_REL[] {
        { .key_= "opcode",  .related_= G_S_BIN_OPC   },
        { .key_= "funct3",  .related_= G_S_BIN_F3    },
        { .key_= "imm12",   .related_= G_B_BIN_IMM12 },
        { .key_= "imm10_5", .related_= G_B_BIN_IMM10 },
        { .key_= "imm4_1",  .related_= G_B_BIN_IMM4  },
        { .key_= "imm11",   .related_= G_B_BIN_IMM11 },
        { .key_= "rs1",     .related_= G_SELF_RS1    },
        { .key_= "rs2",     .related_= G_SELF_RS2    },
    };
    constexpr FieldRel G_B_ASM_REL[] {
        { .key_= "mnemonic", .related_= G_S_ASM_MNE },
        { .key_= "rs1",      .related_= G_SELF_RS1  },
        { .key_= "rs2",      .related_= G_SELF_RS2  },
        { .key_= "imm",      .related_= G_B_ASM_IMM },
    };

} // namespace

const InstFormatView &GetIInstFormatView(ItypeImmShape shape)
{
    static const InstFormatView K_I {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_I_ASM,
        .fields_   = GetIInstFields(ItypeImmShape::IMM12),
        .binaryRel_= G_I_BIN_REL,
        .asmRel_   = G_I_ASM_REL,
        .keyFields_= G_I_KEY,
    };
    static const InstFormatView K_I_SHIFT {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_I_ASM,
        .fields_   = GetIInstFields(ItypeImmShape::SHIFT),
        .binaryRel_= G_I_SHIFT_BIN_REL,
        .asmRel_   = G_I_SHIFT_ASM_REL,
        .keyFields_= G_I_SHIFT_KEY,
    };
    static const InstFormatView K_I_FENCE {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_I_FENCE_ASM,
        .fields_   = GetIInstFields(ItypeImmShape::FENCE),
        .binaryRel_= G_I_FENCE_BIN_REL,
        .asmRel_   = G_I_FENCE_ASM_REL,
        .keyFields_= G_I_KEY,
    };
    static const InstFormatView K_I_CSR {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_I_CSR_ASM,
        .fields_   = GetIInstFields(ItypeImmShape::CSR),
        .binaryRel_= G_I_CSR_BIN_REL,
        .asmRel_   = G_I_CSR_ASM_REL,
        .keyFields_= G_I_KEY,
    };

    switch(shape) {
    case ItypeImmShape::SHIFT: return K_I_SHIFT;
    case ItypeImmShape::FENCE: return K_I_FENCE;
    case ItypeImmShape::CSR:   return K_I_CSR;
    default:                   return K_I;
    }
}

const InstFormatView &GetRInstFormatView(RtypeShape shape)
{
    static const InstFormatView K_R {
        .typeName_ = InstFormatName(InstFormat::R),
        .fmt_      = InstFormat::R,
        .asmTokens_= G_R_ASM,
        .fields_   = GetRInstFields(RtypeShape::BASE),
        .binaryRel_= G_R_BIN_REL,
        .asmRel_   = G_R_ASM_REL,
        .keyFields_= G_R_KEY,
    };
    static const InstFormatView K_R_AMO {
        .typeName_ = InstFormatName(InstFormat::R),
        .fmt_      = InstFormat::R,
        .asmTokens_= G_R_ASM,
        .fields_   = GetRInstFields(RtypeShape::AMO),
        .binaryRel_= G_R_AMO_BIN_REL,
        .asmRel_   = G_R_AMO_ASM_REL,
        .keyFields_= G_R_AMO_KEY,
    };
    static const InstFormatView K_R_FP {
        .typeName_ = InstFormatName(InstFormat::R),
        .fmt_      = InstFormat::R,
        .asmTokens_= G_R_ASM,
        .fields_   = GetRInstFields(RtypeShape::FP),
        .binaryRel_= G_R_FP_BIN_REL,
        .asmRel_   = G_R_FP_ASM_REL,
        .keyFields_= G_R_FP_KEY,
    };
    static const InstFormatView K_R_R4 {
        .typeName_ = InstFormatName(InstFormat::R),
        .fmt_      = InstFormat::R,
        .asmTokens_= G_R_R4_ASM,
        .fields_   = GetRInstFields(RtypeShape::R4),
        .binaryRel_= G_R_R4_BIN_REL,
        .asmRel_   = G_R_R4_ASM_REL,
        .keyFields_= G_R_R4_KEY,
    };

    switch(shape) {
    case RtypeShape::AMO: return K_R_AMO;
    case RtypeShape::FP:  return K_R_FP;
    case RtypeShape::R4:  return K_R_R4;
    default:              return K_R;
    }
}

const InstFormatView &GetInstFormatView(InstFormat fmt)
{
    static const InstFormatView K_J {
        .typeName_ = InstFormatName(InstFormat::J),
        .fmt_      = InstFormat::J,
        .asmTokens_= G_J_ASM,
        .fields_   = GetInstFields(InstFormat::J),
        .binaryRel_= G_J_BIN_REL,
        .asmRel_   = G_J_ASM_REL,
        .keyFields_= G_J_KEY,
    };
    static const InstFormatView K_U {
        .typeName_ = InstFormatName(InstFormat::U),
        .fmt_      = InstFormat::U,
        .asmTokens_= G_U_ASM,
        .fields_   = GetInstFields(InstFormat::U),
        .binaryRel_= G_U_BIN_REL,
        .asmRel_   = G_U_ASM_REL,
        .keyFields_= G_U_KEY,
    };
    static const InstFormatView K_S {
        .typeName_ = InstFormatName(InstFormat::S),
        .fmt_      = InstFormat::S,
        .asmTokens_= G_S_ASM,
        .fields_   = GetInstFields(InstFormat::S),
        .binaryRel_= G_S_BIN_REL,
        .asmRel_   = G_S_ASM_REL,
        .keyFields_= G_S_KEY,
    };
    static const InstFormatView K_B {
        .typeName_ = InstFormatName(InstFormat::B),
        .fmt_      = InstFormat::B,
        .asmTokens_= G_B_ASM,
        .fields_   = GetInstFields(InstFormat::B),
        .binaryRel_= G_B_BIN_REL,
        .asmRel_   = G_B_ASM_REL,
        .keyFields_= G_S_KEY,
    };
    static const InstFormatView K_C {
        .typeName_ = InstFormatName(InstFormat::C),
        .fmt_      = InstFormat::C,
        .asmTokens_= G_C_ASM,
        .fields_   = GetInstFields(InstFormat::C),
        .binaryRel_= {},
        .asmRel_   = {},
        .keyFields_= G_C_KEY,
    };
    static const InstFormatView K_UNKNOWN {};

    switch(fmt) {
    case InstFormat::R: return GetRInstFormatView(RtypeShape::BASE);
    case InstFormat::I: return GetIInstFormatView(ItypeImmShape::IMM12);
    case InstFormat::J: return K_J;
    case InstFormat::U: return K_U;
    case InstFormat::S: return K_S;
    case InstFormat::B: return K_B;
    case InstFormat::C: return K_C;
    default:            return K_UNKNOWN;
    }
}

} // namespace util
