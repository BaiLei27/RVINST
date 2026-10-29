#pragma once

#include "ISA/InstFormat.hh"

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

[[nodiscard]] constexpr std::span<const std::string_view>
    FIND_FIELD_REL(std::span<const FieldRel> rels, std::string_view key) noexcept
{
    for(const auto &r: rels) {
        if(r.key_ == key) return r.related_;
    }
    return {};
}

[[nodiscard]] constexpr bool IS_KEY_FIELD(std::span<const std::string_view> keyFields,
                                          std::string_view name) noexcept
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
    if(name == "rs2" || name == "rs3" || name == "fm") return FieldTone::RS2;
    if(name.starts_with("imm") || name == "shamt" || name == "csr") return FieldTone::IMM;
    // funct5/fmt/aq/rl/rm are decode-key / encoding fields → KEY tone
    return FieldTone::KEY;
}

[[nodiscard]] constexpr FieldTone ASM_TOKEN_TONE(std::string_view token) noexcept
{
    if(token == "mnemonic") return FieldTone::KEY;
    if(token == "rd" || token == "pred") return FieldTone::RD;
    if(token == "rs1" || token == "succ") return FieldTone::RS1;
    if(token == "rs2") return FieldTone::RS2;
    if(token == "imm" || token == "csr") return FieldTone::IMM;
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

[[nodiscard]] const InstFormatView &GetIInstFormatView(IImmShape shape);
[[nodiscard]] const InstFormatView &GetRInstFormatView(RShape shape);
[[nodiscard]] const InstFormatView &GetInstFormatView(InstFormat fmt);

} // namespace util

// update:26-10-12-00:17
