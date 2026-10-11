#pragma once

#include <span>
#include <string_view>

#include "ISA/InstFormat.hh"

// #include "../ISA/InstFormat.hh" // ! clangd bug

namespace util {

enum class FieldTone : uint8_t {
    KEY,
    RD,
    RS1,
    RS2,
    IMM
};

struct FieldRel {
    std::string_view key_;
    std::span<const std::string_view> related_;
};

struct InstFormatView {
    std::string_view typeName_;
    InstFormat fmt_ { InstFormat::UNKNOWN };
    std::span<const std::string_view> asmTokens_;
    std::span<const InstField> fields_;
    std::span<const FieldRel> binaryRel_;
    std::span<const FieldRel> asmRel_;
    std::span<const std::string_view> keyFields_; // fields that form keyID_ (+ mnemonic)
};

[[nodiscard]]
constexpr std::span<const std::string_view> FIND_FIELD_REL(std::span<const FieldRel> rels, std::string_view key) noexcept
{
    for(const auto &r: rels) {
        if(r.key_ == key) return r.related_;
    }
    return {};
}

[[nodiscard]] constexpr bool IS_KEY_FIELD(std::span<const std::string_view> keyFields, std::string_view name) noexcept
{
    for(const auto &k: keyFields) {
        if(k == name) return true;
    }
    return false;
}

[[nodiscard]] constexpr bool HAS_NAMED_FIELD(std::span<const InstField> fields,
                                             std::string_view name) noexcept
{
    for(const auto &f: fields) {
        if(f.name_ == name) return true;
    }
    return false;
}

[[nodiscard]] constexpr FieldTone BINARY_FIELD_TONE(std::string_view name,
                                                    std::span<const std::string_view> keyFields,
                                                    std::span<const InstField> fields= {}) noexcept
{
    if(IS_KEY_FIELD(keyFields, name)) return FieldTone::KEY;
    if(name == "rd" || name == "pred") return FieldTone::RD;
    if(name == "succ") return FieldTone::RS1;
    // fence diagram also has succ (purple); use imm blue so rs1 is distinct
    if(name == "rs1") return HAS_NAMED_FIELD(fields, "succ") ? FieldTone::IMM : FieldTone::RS1;
    if(name == "rs2" || name == "fm") return FieldTone::RS2; // fence mode; not in asm operands
    if(name.starts_with("imm") || name == "shamt") return FieldTone::IMM;
    return FieldTone::KEY;
}

[[nodiscard]] constexpr FieldTone ASM_TOKEN_TONE(std::string_view token) noexcept
{
    if(token == "mnemonic") return FieldTone::KEY;
    if(token == "rd" || token == "pred") return FieldTone::RD;
    if(token == "rs1" || token == "succ") return FieldTone::RS1;
    if(token == "rs2") return FieldTone::RS2;
    if(token == "imm") return FieldTone::IMM;
    return FieldTone::KEY;
}

[[nodiscard]] constexpr const char *FIELD_TONE_CLASS(FieldTone tone) noexcept
{
    switch(tone) {
    case FieldTone::KEY: return "bit-tone-key";
    case FieldTone::RD:  return "bit-tone-rd";
    case FieldTone::RS1: return "bit-tone-rs1";
    case FieldTone::RS2: return "bit-tone-rs2";
    case FieldTone::IMM: return "bit-tone-imm";
    }
    return "bit-tone-key";
}

namespace detail {

    inline constexpr std::string_view G_kRAsm[] { "mnemonic", "rd", ",", "rs1", ",", "rs2" };
    inline constexpr std::string_view G_kIAsm[] { "mnemonic", "rd", ",", "rs1", ",", "imm" };
    inline constexpr std::string_view G_kIFenceAsm[] { "mnemonic", "pred", ",", "succ" };
    inline constexpr std::string_view G_kJAsm[] { "mnemonic", "rd", ",", "imm" };
    inline constexpr std::string_view G_kUAsm[] { "mnemonic", "rd", ",", "imm" };
    inline constexpr std::string_view G_kSAsm[] { "mnemonic", "rs2", ",", "imm", "(", "rs1", ")" };
    inline constexpr std::string_view G_kBAsm[] { "mnemonic", "rs1", ",", "rs2", ",", "imm" };

    inline constexpr std::string_view G_kRKey[] { "opcode", "funct3", "funct7" };
    inline constexpr std::string_view G_kIKey[] { "opcode", "funct3" };
    inline constexpr std::string_view G_kIShiftKey[] { "opcode", "funct3", "imm7" };
    inline constexpr std::string_view G_kSKey[] { "opcode", "funct3" };
    inline constexpr std::string_view G_kUKey[] { "opcode" };
    inline constexpr std::string_view G_kJKey[] { "opcode" };

    inline constexpr std::string_view G_kRBinOpcode[] { "mnemonic", "funct3", "funct7" };
    inline constexpr std::string_view G_kRBinFunct3[] { "mnemonic", "opcode", "funct7" };
    inline constexpr std::string_view G_kRBinFunct7[] { "mnemonic", "opcode", "funct3" };
    inline constexpr std::string_view G_kSelfRd[] { "rd" };
    inline constexpr std::string_view G_kSelfRs1[] { "rs1" };
    inline constexpr std::string_view G_kSelfRs2[] { "rs2" };
    inline constexpr std::string_view G_kSelfImm[] { "imm" };
    inline constexpr std::string_view G_kRAsmMnemonic[] { "opcode", "funct3", "funct7" };

    inline constexpr std::string_view G_kIBinOpcode[] { "mnemonic", "funct3" };
    inline constexpr std::string_view G_kIBinFunct3[] { "mnemonic", "opcode" };
    inline constexpr std::string_view G_kIBinImm[] { "imm" };
    inline constexpr std::string_view G_kIAsmMnemonic[] { "opcode", "funct3" };
    inline constexpr std::string_view G_kIAsmImm[] { "imm11_0" };

    inline constexpr std::string_view G_kIShiftBinOpcode[] { "mnemonic", "funct3", "imm7" };
    inline constexpr std::string_view G_kIShiftBinFunct3[] { "mnemonic", "opcode", "imm7" };
    inline constexpr std::string_view G_kIShiftBinImm7[] { "mnemonic", "opcode", "funct3" };
    inline constexpr std::string_view G_kIShiftBinShamt[] { "imm" };
    inline constexpr std::string_view G_kIShiftAsmMnemonic[] { "opcode", "funct3", "imm7" };
    inline constexpr std::string_view G_kIShiftAsmImm[] { "imm7", "shamt" };

    inline constexpr std::string_view G_kSelfPred[] { "pred" };
    inline constexpr std::string_view G_kSelfSucc[] { "succ" };

    inline constexpr std::string_view G_kMnemonicOnly[] { "mnemonic" };
    inline constexpr std::string_view G_kJAsmImm[] { "imm20", "imm10_1", "imm11", "imm19_12" };
    inline constexpr std::string_view G_kUAsmImm[] { "imm31_12" };

    inline constexpr std::string_view G_kSBinOpcode[] { "mnemonic", "funct3" };
    inline constexpr std::string_view G_kSBinFunct3[] { "mnemonic", "opcode" };
    inline constexpr std::string_view G_kSAsmMnemonic[] { "opcode", "funct3" };
    inline constexpr std::string_view G_kSAsmImm[] { "imm11_5", "imm4_0" };
    inline constexpr std::string_view G_kSBinImm11[] { "imm", "imm4_0" };
    inline constexpr std::string_view G_kSBinImm4[] { "imm", "imm11_5" };
    inline constexpr std::string_view G_kBAsmImm[] { "imm12", "imm10_5", "imm4_1", "imm11" };
    inline constexpr std::string_view G_kBBinImm12[] { "imm", "imm10_5", "imm4_1", "imm11" };
    inline constexpr std::string_view G_kBBinImm10[] { "imm", "imm12", "imm4_1", "imm11" };
    inline constexpr std::string_view G_kBBinImm4[] { "imm", "imm12", "imm10_5", "imm11" };
    inline constexpr std::string_view G_kBBinImm11[] { "imm", "imm12", "imm10_5", "imm4_1" };
    inline constexpr std::string_view G_kJBinImm20[] { "imm", "imm10_1", "imm11", "imm19_12" };
    inline constexpr std::string_view G_kJBinImm10[] { "imm", "imm20", "imm11", "imm19_12" };
    inline constexpr std::string_view G_kJBinImm11[] { "imm", "imm20", "imm10_1", "imm19_12" };
    inline constexpr std::string_view G_kJBinImm19[] { "imm", "imm20", "imm10_1", "imm11" };

    inline constexpr FieldRel G_kRBinRel[] {
        { .key_= "opcode", .related_= G_kRBinOpcode },
        { .key_= "funct3", .related_= G_kRBinFunct3 },
        { .key_= "funct7", .related_= G_kRBinFunct7 },
        { .key_= "rd",     .related_= G_kSelfRd     },
        { .key_= "rs1",    .related_= G_kSelfRs1    },
        { .key_= "rs2",    .related_= G_kSelfRs2    },
    };
    inline constexpr FieldRel G_kRAsmRel[] {
        { .key_= "mnemonic", .related_= G_kRAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
    };

    inline constexpr FieldRel G_kIBinRel[] {
        { .key_= "opcode",  .related_= G_kIBinOpcode },
        { .key_= "funct3",  .related_= G_kIBinFunct3 },
        { .key_= "imm11_0", .related_= G_kIBinImm    },
        { .key_= "rd",      .related_= G_kSelfRd     },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
    };
    inline constexpr FieldRel G_kIAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "imm",      .related_= G_kIAsmImm      },
    };

    inline constexpr FieldRel G_kIShiftBinRel[] {
        { .key_= "opcode", .related_= G_kIShiftBinOpcode },
        { .key_= "funct3", .related_= G_kIShiftBinFunct3 },
        { .key_= "imm7",   .related_= G_kIShiftBinImm7   },
        { .key_= "shamt",  .related_= G_kIShiftBinShamt  },
        { .key_= "rd",     .related_= G_kSelfRd          },
        { .key_= "rs1",    .related_= G_kSelfRs1         },
    };
    inline constexpr FieldRel G_kIShiftAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIShiftAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd            },
        { .key_= "rs1",      .related_= G_kSelfRs1           },
        { .key_= "imm",      .related_= G_kIShiftAsmImm      },
    };

    inline constexpr FieldRel G_kIFenceBinRel[] {
        { .key_= "opcode", .related_= G_kIBinOpcode },
        { .key_= "funct3", .related_= G_kIBinFunct3 },
        { .key_= "pred",   .related_= G_kSelfPred   },
        { .key_= "succ",   .related_= G_kSelfSucc   },
        { .key_= "rd",     .related_= G_kSelfRd     },
        { .key_= "rs1",    .related_= G_kSelfRs1    },
    };
    inline constexpr FieldRel G_kIFenceAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIAsmMnemonic },
        { .key_= "pred",     .related_= G_kSelfPred     },
        { .key_= "succ",     .related_= G_kSelfSucc     },
    };

    inline constexpr FieldRel G_kJBinRel[] {
        { .key_= "opcode",   .related_= G_kMnemonicOnly },
        { .key_= "imm20",    .related_= G_kJBinImm20    },
        { .key_= "imm10_1",  .related_= G_kJBinImm10    },
        { .key_= "imm11",    .related_= G_kJBinImm11    },
        { .key_= "imm19_12", .related_= G_kJBinImm19    },
        { .key_= "rd",       .related_= G_kSelfRd       },
    };
    inline constexpr FieldRel G_kJAsmRel[] {
        { .key_= "mnemonic", .related_= G_kMnemonicOnly },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "imm",      .related_= G_kJAsmImm      },
    };

    inline constexpr FieldRel G_kUBinRel[] {
        { .key_= "opcode",   .related_= G_kMnemonicOnly },
        { .key_= "imm31_12", .related_= G_kSelfImm      },
        { .key_= "rd",       .related_= G_kSelfRd       },
    };
    inline constexpr FieldRel G_kUAsmRel[] {
        { .key_= "mnemonic", .related_= G_kMnemonicOnly },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "imm",      .related_= G_kUAsmImm      },
    };

    inline constexpr FieldRel G_kSBinRel[] {
        { .key_= "opcode",  .related_= G_kSBinOpcode },
        { .key_= "funct3",  .related_= G_kSBinFunct3 },
        { .key_= "imm11_5", .related_= G_kSBinImm11  },
        { .key_= "imm4_0",  .related_= G_kSBinImm4   },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
        { .key_= "rs2",     .related_= G_kSelfRs2    },
    };
    inline constexpr FieldRel G_kSAsmRel[] {
        { .key_= "mnemonic", .related_= G_kSAsmMnemonic },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
        { .key_= "imm",      .related_= G_kSAsmImm      },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
    };

    inline constexpr FieldRel G_kBBinRel[] {
        { .key_= "opcode",  .related_= G_kSBinOpcode },
        { .key_= "funct3",  .related_= G_kSBinFunct3 },
        { .key_= "imm12",   .related_= G_kBBinImm12  },
        { .key_= "imm10_5", .related_= G_kBBinImm10  },
        { .key_= "imm4_1",  .related_= G_kBBinImm4   },
        { .key_= "imm11",   .related_= G_kBBinImm11  },
        { .key_= "rs1",     .related_= G_kSelfRs1    },
        { .key_= "rs2",     .related_= G_kSelfRs2    },
    };
    inline constexpr FieldRel G_kBAsmRel[] {
        { .key_= "mnemonic", .related_= G_kSAsmMnemonic },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "rs2",      .related_= G_kSelfRs2      },
        { .key_= "imm",      .related_= G_kBAsmImm      },
    };

} // namespace detail

[[nodiscard]] inline const InstFormatView &GetIInstFormatView(ItypeImmShape shape)
{
    using namespace detail; // NOLINT
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
    default:               return K_I;
    }
}

[[nodiscard]] inline const InstFormatView &GetInstFormatView(InstFormat fmt)
{
    using namespace detail; // NOLINT
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
