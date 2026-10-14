#include "Util/InstFormatView.hh"

namespace util {
namespace {

    constexpr std::string_view G_kRAsm[] { "mnemonic", "rd", ",", "rs1", ",", "rs2" };
    constexpr std::string_view G_kIAsm[] { "mnemonic", "rd", ",", "rs1", ",", "imm" };
    constexpr std::string_view G_kIFenceAsm[] { "mnemonic", "pred", ",", "succ" };
    constexpr std::string_view G_kJAsm[] { "mnemonic", "rd", ",", "imm" };
    constexpr std::string_view G_kUAsm[] { "mnemonic", "rd", ",", "imm" };
    constexpr std::string_view G_kSAsm[] { "mnemonic", "rs2", ",", "imm", "(", "rs1", ")" };
    constexpr std::string_view G_kBAsm[] { "mnemonic", "rs1", ",", "rs2", ",", "imm" };

    constexpr std::string_view G_kRKey[] { "opcode", "funct3", "funct7" };
    constexpr std::string_view G_kIKey[] { "opcode", "funct3" };
    constexpr std::string_view G_kIShiftKey[] { "opcode", "funct3", "imm7" };
    constexpr std::string_view G_kSKey[] { "opcode", "funct3" };
    constexpr std::string_view G_kUKey[] { "opcode" };
    constexpr std::string_view G_kJKey[] { "opcode" };

    constexpr std::string_view G_kRBinOpcode[] { "mnemonic", "funct3", "funct7" };
    constexpr std::string_view G_kRBinFunct3[] { "mnemonic", "opcode", "funct7" };
    constexpr std::string_view G_kRBinFunct7[] { "mnemonic", "opcode", "funct3" };
    constexpr std::string_view G_kSelfRd[] { "rd" };
    constexpr std::string_view G_kSelfRs1[] { "rs1" };
    constexpr std::string_view G_kSelfRs2[] { "rs2" };
    constexpr std::string_view G_kSelfImm[] { "imm" };
    constexpr std::string_view G_kRAsmMnemonic[] { "opcode", "funct3", "funct7" };

    constexpr std::string_view G_kIBinOpcode[] { "mnemonic", "funct3" };
    constexpr std::string_view G_kIBinFunct3[] { "mnemonic", "opcode" };
    constexpr std::string_view G_kIBinImm[] { "imm" };
    constexpr std::string_view G_kIAsmMnemonic[] { "opcode", "funct3" };
    constexpr std::string_view G_kIAsmImm[] { "imm11_0" };

    constexpr std::string_view G_kIShiftBinOpcode[] { "mnemonic", "funct3", "imm7" };
    constexpr std::string_view G_kIShiftBinFunct3[] { "mnemonic", "opcode", "imm7" };
    constexpr std::string_view G_kIShiftBinImm7[] { "mnemonic", "opcode", "funct3" };
    constexpr std::string_view G_kIShiftBinShamt[] { "imm" };
    constexpr std::string_view G_kIShiftAsmMnemonic[] { "opcode", "funct3", "imm7" };
    constexpr std::string_view G_kIShiftAsmImm[] { "imm7", "shamt" };

    constexpr std::string_view G_kSelfPred[] { "pred" };
    constexpr std::string_view G_kSelfSucc[] { "succ" };

    constexpr std::string_view G_kMnemonicOnly[] { "mnemonic" };
    constexpr std::string_view G_kJAsmImm[] { "imm20", "imm10_1", "imm11", "imm19_12" };
    constexpr std::string_view G_kUAsmImm[] { "imm31_12" };

    constexpr std::string_view G_kSBinOpcode[] { "mnemonic", "funct3" };
    constexpr std::string_view G_kSBinFunct3[] { "mnemonic", "opcode" };
    constexpr std::string_view G_kSAsmMnemonic[] { "opcode", "funct3" };
    constexpr std::string_view G_kSAsmImm[] { "imm11_5", "imm4_0" };
    constexpr std::string_view G_kSBinImm11[] { "imm", "imm4_0" };
    constexpr std::string_view G_kSBinImm4[] { "imm", "imm11_5" };
    constexpr std::string_view G_kBAsmImm[] { "imm12", "imm10_5", "imm4_1", "imm11" };
    constexpr std::string_view G_kBBinImm12[] { "imm", "imm10_5", "imm4_1", "imm11" };
    constexpr std::string_view G_kBBinImm10[] { "imm", "imm12", "imm4_1", "imm11" };
    constexpr std::string_view G_kBBinImm4[] { "imm", "imm12", "imm10_5", "imm11" };
    constexpr std::string_view G_kBBinImm11[] { "imm", "imm12", "imm10_5", "imm4_1" };
    constexpr std::string_view G_kJBinImm20[] { "imm", "imm10_1", "imm11", "imm19_12" };
    constexpr std::string_view G_kJBinImm10[] { "imm", "imm20", "imm11", "imm19_12" };
    constexpr std::string_view G_kJBinImm11[] { "imm", "imm20", "imm10_1", "imm19_12" };
    constexpr std::string_view G_kJBinImm19[] { "imm", "imm20", "imm10_1", "imm11" };

    constexpr FieldRel G_kRBinRel[] {
        { .key_= "opcode", .related_= G_kRBinOpcode },
        { .key_= "funct3", .related_= G_kRBinFunct3 },
        { .key_= "funct7", .related_= G_kRBinFunct7 },
        { .key_= "rd",     .related_= G_kSelfRd     },
        { .key_= "rs1",    .related_= G_kSelfRs1    },
        { .key_= "rs2",    .related_= G_kSelfRs2    },
    };
    constexpr FieldRel G_kRAsmRel[] {
        { .key_= "mnemonic", .related_= G_kRAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
    };

    constexpr FieldRel G_kIBinRel[] {
        { .key_= "opcode",  .related_= G_kIBinOpcode },
        { .key_= "funct3",  .related_= G_kIBinFunct3 },
        { .key_= "imm11_0", .related_= G_kIBinImm    },
        { .key_= "rd",      .related_= G_kSelfRd     },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
    };
    constexpr FieldRel G_kIAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "imm",      .related_= G_kIAsmImm      },
    };

    constexpr FieldRel G_kIShiftBinRel[] {
        { .key_= "opcode", .related_= G_kIShiftBinOpcode },
        { .key_= "funct3", .related_= G_kIShiftBinFunct3 },
        { .key_= "imm7",   .related_= G_kIShiftBinImm7   },
        { .key_= "shamt",  .related_= G_kIShiftBinShamt  },
        { .key_= "rd",     .related_= G_kSelfRd          },
        { .key_= "rs1",    .related_= G_kSelfRs1         },
    };
    constexpr FieldRel G_kIShiftAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIShiftAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd            },
        { .key_= "rs1",      .related_= G_kSelfRs1           },
        { .key_= "imm",      .related_= G_kIShiftAsmImm      },
    };

    constexpr FieldRel G_kIFenceBinRel[] {
        { .key_= "opcode", .related_= G_kIBinOpcode },
        { .key_= "funct3", .related_= G_kIBinFunct3 },
        { .key_= "pred",   .related_= G_kSelfPred   },
        { .key_= "succ",   .related_= G_kSelfSucc   },
        { .key_= "rd",     .related_= G_kSelfRd     },
        { .key_= "rs1",    .related_= G_kSelfRs1    },
    };
    constexpr FieldRel G_kIFenceAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIAsmMnemonic },
        { .key_= "pred",     .related_= G_kSelfPred     },
        { .key_= "succ",     .related_= G_kSelfSucc     },
    };

    constexpr FieldRel G_kJBinRel[] {
        { .key_= "opcode",   .related_= G_kMnemonicOnly },
        { .key_= "imm20",    .related_= G_kJBinImm20    },
        { .key_= "imm10_1",  .related_= G_kJBinImm10    },
        { .key_= "imm11",    .related_= G_kJBinImm11    },
        { .key_= "imm19_12", .related_= G_kJBinImm19    },
        { .key_= "rd",       .related_= G_kSelfRd       },
    };
    constexpr FieldRel G_kJAsmRel[] {
        { .key_= "mnemonic", .related_= G_kMnemonicOnly },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "imm",      .related_= G_kJAsmImm      },
    };

    constexpr FieldRel G_kUBinRel[] {
        { .key_= "opcode",   .related_= G_kMnemonicOnly },
        { .key_= "imm31_12", .related_= G_kSelfImm      },
        { .key_= "rd",       .related_= G_kSelfRd       },
    };
    constexpr FieldRel G_kUAsmRel[] {
        { .key_= "mnemonic", .related_= G_kMnemonicOnly },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "imm",      .related_= G_kUAsmImm      },
    };

    constexpr FieldRel G_kSBinRel[] {
        { .key_= "opcode",  .related_= G_kSBinOpcode },
        { .key_= "funct3",  .related_= G_kSBinFunct3 },
        { .key_= "imm11_5", .related_= G_kSBinImm11  },
        { .key_= "imm4_0",  .related_= G_kSBinImm4   },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
        { .key_= "rs2",     .related_= G_kSelfRs2    },
    };
    constexpr FieldRel G_kSAsmRel[] {
        { .key_= "mnemonic", .related_= G_kSAsmMnemonic },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
        { .key_= "imm",      .related_= G_kSAsmImm      },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
    };

    constexpr FieldRel G_kBBinRel[] {
        { .key_= "opcode",  .related_= G_kSBinOpcode },
        { .key_= "funct3",  .related_= G_kSBinFunct3 },
        { .key_= "imm12",   .related_= G_kBBinImm12  },
        { .key_= "imm10_5", .related_= G_kBBinImm10  },
        { .key_= "imm4_1",  .related_= G_kBBinImm4   },
        { .key_= "imm11",   .related_= G_kBBinImm11  },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
        { .key_= "rs2",     .related_= G_kSelfRs2    },
    };
    constexpr FieldRel G_kBAsmRel[] {
        { .key_= "mnemonic", .related_= G_kSAsmMnemonic },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
        { .key_= "imm",      .related_= G_kBAsmImm      },
    };

} // namespace

const InstFormatView &GetIInstFormatView(ItypeImmShape shape)
{
    static const InstFormatView K_I {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_kIAsm,
        .fields_   = GetIInstFields(ItypeImmShape::IMM12),
        .binaryRel_= G_kIBinRel,
        .asmRel_   = G_kIAsmRel,
        .keyFields_= G_kIKey,
    };
    static const InstFormatView K_I_SHIFT {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_kIAsm,
        .fields_   = GetIInstFields(ItypeImmShape::SHIFT),
        .binaryRel_= G_kIShiftBinRel,
        .asmRel_   = G_kIShiftAsmRel,
        .keyFields_= G_kIShiftKey,
    };
    static const InstFormatView K_I_FENCE {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_kIFenceAsm,
        .fields_   = GetIInstFields(ItypeImmShape::FENCE),
        .binaryRel_= G_kIFenceBinRel,
        .asmRel_   = G_kIFenceAsmRel,
        .keyFields_= G_kIKey,
    };

    switch(shape) {
    case ItypeImmShape::SHIFT: return K_I_SHIFT;
    case ItypeImmShape::FENCE: return K_I_FENCE;
    default:                   return K_I;
    }
}

const InstFormatView &GetInstFormatView(InstFormat fmt)
{
    static const InstFormatView K_R {
        .typeName_ = InstFormatName(InstFormat::R),
        .fmt_      = InstFormat::R,
        .asmTokens_= G_kRAsm,
        .fields_   = GetInstFields(InstFormat::R),
        .binaryRel_= G_kRBinRel,
        .asmRel_   = G_kRAsmRel,
        .keyFields_= G_kRKey,
    };
    static const InstFormatView K_J {
        .typeName_ = InstFormatName(InstFormat::J),
        .fmt_      = InstFormat::J,
        .asmTokens_= G_kJAsm,
        .fields_   = GetInstFields(InstFormat::J),
        .binaryRel_= G_kJBinRel,
        .asmRel_   = G_kJAsmRel,
        .keyFields_= G_kJKey,
    };
    static const InstFormatView K_U {
        .typeName_ = InstFormatName(InstFormat::U),
        .fmt_      = InstFormat::U,
        .asmTokens_= G_kUAsm,
        .fields_   = GetInstFields(InstFormat::U),
        .binaryRel_= G_kUBinRel,
        .asmRel_   = G_kUAsmRel,
        .keyFields_= G_kUKey,
    };
    static const InstFormatView K_S {
        .typeName_ = InstFormatName(InstFormat::S),
        .fmt_      = InstFormat::S,
        .asmTokens_= G_kSAsm,
        .fields_   = GetInstFields(InstFormat::S),
        .binaryRel_= G_kSBinRel,
        .asmRel_   = G_kSAsmRel,
        .keyFields_= G_kSKey,
    };
    static const InstFormatView K_B {
        .typeName_ = InstFormatName(InstFormat::B),
        .fmt_      = InstFormat::B,
        .asmTokens_= G_kBAsm,
        .fields_   = GetInstFields(InstFormat::B),
        .binaryRel_= G_kBBinRel,
        .asmRel_   = G_kBAsmRel,
        .keyFields_= G_kSKey,
    };
    static const InstFormatView K_UNKNOWN {};

    switch(fmt) {
    case InstFormat::R: return K_R;
    case InstFormat::I: return GetIInstFormatView(ItypeImmShape::IMM12);
    case InstFormat::J: return K_J;
    case InstFormat::U: return K_U;
    case InstFormat::S: return K_S;
    case InstFormat::B: return K_B;
    default:            return K_UNKNOWN;
    }
}

} // namespace util
