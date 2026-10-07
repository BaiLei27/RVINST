#pragma once

#include <span>
#include <string_view>

#include "ISA/InstFormat.hh"

// #include "../ISA/InstFormat.hh" // ! clangd bug

namespace util {

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
};

[[nodiscard]]
constexpr std::span<const std::string_view> FIND_FIELD_REL(std::span<const FieldRel> rels, std::string_view key) noexcept
{
    for(const auto &r: rels) {
        if(r.key_ == key)
            return r.related_;
    }
    return {};
}

namespace detail {

    inline constexpr std::string_view G_kRAsm[] { "mnemonic", "rd", ",", "rs1", ",", "rs2" };
    inline constexpr std::string_view G_kIAsm[] { "mnemonic", "rd", ",", "rs1", ",", "imm" };
    inline constexpr std::string_view G_kJAsm[] { "mnemonic", "rd", ",", "imm" };
    inline constexpr std::string_view G_kUAsm[] { "mnemonic", "rd", ",", "imm" };
    inline constexpr std::string_view G_kSAsm[] { "mnemonic", "rs2", ",", "imm", "(", "rs1", ")" };
    inline constexpr std::string_view G_kBAsm[] { "mnemonic", "rs1", ",", "rs2", ",", "imm" };

    inline constexpr std::string_view G_kRBinOpcode[] { "mnemonic", "funct3", "funct7" };
    inline constexpr std::string_view G_kRBinFunct3[] { "mnemonic", "opcode", "funct7" };
    inline constexpr std::string_view G_kRBinFunct7[] { "mnemonic", "opcode", "funct3" };
    inline constexpr std::string_view G_kSelfRd[] { "rd" };
    inline constexpr std::string_view G_kSelfRs1[] { "rs1" };
    inline constexpr std::string_view G_kSelfRs2[] { "rs2" };
    inline constexpr std::string_view G_kSelfImm[] { "imm" };
    inline constexpr std::string_view G_kRAsmMnemonic[] { "opcode", "funct3", "funct7" };

    inline constexpr std::string_view G_kIBinOpcode[] { "mnemonic", "funct3", "imm" };
    inline constexpr std::string_view G_kIBinFunct3[] { "mnemonic", "opcode", "imm" };
    inline constexpr std::string_view G_kIBinImm[] { "mnemonic", "opcode", "funct3" };
    inline constexpr std::string_view G_kIAsmMnemonic[] { "opcode", "funct3", "imm" };

    inline constexpr std::string_view G_kMnemonicOnly[] { "mnemonic" };
    inline constexpr std::string_view G_kJAsmImm[] { "imm20", "imm10_1", "imm11", "imm19_12" };
    inline constexpr std::string_view G_kUAsmImm[] { "imm31_12" };

    inline constexpr std::string_view G_kSBinOpcode[] { "mnemonic", "funct3" };
    inline constexpr std::string_view G_kSBinFunct3[] { "mnemonic", "opcode" };
    inline constexpr std::string_view G_kSAsmMnemonic[] { "opcode", "funct3" };
    inline constexpr std::string_view G_kSAsmImm[] { "imm11_5", "imm4_0" };
    inline constexpr std::string_view G_kBAsmImm[] { "imm12", "imm10_5", "imm4_1", "imm11" };

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
        { .key_= "opcode", .related_= G_kIBinOpcode },
        { .key_= "funct3", .related_= G_kIBinFunct3 },
        { .key_= "imm",    .related_= G_kIBinImm    },
        { .key_= "rd",     .related_= G_kSelfRd     },
        { .key_= "rs1",    .related_= G_kSelfRs1    },
    };
    inline constexpr FieldRel G_kIAsmRel[] {
        { .key_= "mnemonic", .related_= G_kIAsmMnemonic },
        { .key_= "rd",       .related_= G_kSelfRd       },
        { .key_= "rs1",      .related_= G_kSelfRs1      },
        { .key_= "imm",      .related_= G_kSelfImm      },
    };

    inline constexpr FieldRel G_kJBinRel[] {
        { .key_= "opcode",   .related_= G_kMnemonicOnly },
        { .key_= "imm20",    .related_= G_kSelfImm      },
        { .key_= "imm10_1",  .related_= G_kSelfImm      },
        { .key_= "imm11",    .related_= G_kSelfImm      },
        { .key_= "imm19_12", .related_= G_kSelfImm      },
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
        { .key_= "imm11_5", .related_= G_kSelfImm    },
        { .key_= "imm4_0",  .related_= G_kSelfImm    },
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
        { .key_= "imm12",   .related_= G_kSelfImm    },
        { .key_= "imm10_5", .related_= G_kSelfImm    },
        { .key_= "imm4_1",  .related_= G_kSelfImm    },
        { .key_= "imm11",   .related_= G_kSelfImm    },
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
    };
    static const InstFormatView K_I {
        .typeName_ = InstFormatName(InstFormat::I),
        .fmt_      = InstFormat::I,
        .asmTokens_= G_kIAsm,
        .fields_   = GetInstFields(InstFormat::I),
        .binaryRel_= G_kIBinRel,
        .asmRel_   = G_kIAsmRel,
    };
    static const InstFormatView K_J {
        .typeName_ = InstFormatName(InstFormat::J),
        .fmt_      = InstFormat::J,
        .asmTokens_= G_kJAsm,
        .fields_   = GetInstFields(InstFormat::J),
        .binaryRel_= G_kJBinRel,
        .asmRel_   = G_kJAsmRel,
    };
    static const InstFormatView K_U {
        .typeName_ = InstFormatName(InstFormat::U),
        .fmt_      = InstFormat::U,
        .asmTokens_= G_kUAsm,
        .fields_   = GetInstFields(InstFormat::U),
        .binaryRel_= G_kUBinRel,
        .asmRel_   = G_kUAsmRel,
    };
    static const InstFormatView K_S {
        .typeName_ = InstFormatName(InstFormat::S),
        .fmt_      = InstFormat::S,
        .asmTokens_= G_kSAsm,
        .fields_   = GetInstFields(InstFormat::S),
        .binaryRel_= G_kSBinRel,
        .asmRel_   = G_kSAsmRel,
    };
    static const InstFormatView K_B {
        .typeName_ = InstFormatName(InstFormat::B),
        .fmt_      = InstFormat::B,
        .asmTokens_= G_kBAsm,
        .fields_   = GetInstFields(InstFormat::B),
        .binaryRel_= G_kBBinRel,
        .asmRel_   = G_kBAsmRel,
    };
    static const InstFormatView K_UNKNOWN {};

    switch(fmt) {
    case InstFormat::R: return K_R;
    case InstFormat::I: return K_I;
    case InstFormat::J: return K_J;
    case InstFormat::U: return K_U;
    case InstFormat::S: return K_S;
    case InstFormat::B: return K_B;
    default:            return K_UNKNOWN;
    }
}

} // namespace util
