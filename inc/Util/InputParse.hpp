#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace util {

enum class InputKind : uint8_t {
    HEX,
    BINARY,
    ASSEMBLY,
    UNKNOWN
};

struct ParsedInstInput {
    InputKind kind_ { InputKind::ASSEMBLY };
    uint32_t word_ {};
};

[[nodiscard]] constexpr std::string_view STRIP_INT_PREFIX(std::string_view s) noexcept
{
    if(s.size() >= 2
       && (s.starts_with("0x")
           || s.starts_with("0X")
           || s.starts_with("0b")
           || s.starts_with("0B"))) return s.substr(2);

    return s;
}

[[nodiscard]] inline bool LooksLikeHex(std::string_view s)
{
    if(s.empty()) return false;

    auto digits= (s.size() >= 2 && (s.starts_with("0x") || s.starts_with("0X"))) ? s.substr(2) : s;

    if(digits.empty()) return false;

    return std::ranges::all_of(digits, [](unsigned char c) { return std::isxdigit(c) != 0; });
}

[[nodiscard]] inline bool LooksLikeBinary(std::string_view s)
{
    if(s.empty()) return false;

    std::string_view digits(s);
    if(s.size() >= 2 && (s.starts_with("0b") || s.starts_with("0B"))) {
        digits= s.substr(2);
    } else if(digits.size() < 2) {
        return false;
    }
    if(digits.empty() || digits.size() > 32) return false;

    return std::ranges::all_of(digits, [](unsigned char c) { return c == '0' || c == '1'; });
}

[[nodiscard]] inline ParsedInstInput ClassifyInstInput(std::string_view input)
{
    ParsedInstInput r;
    auto parseword= [](std::string_view digits, int base) {
        auto v= std::stoull(std::string(digits), nullptr, base);
        if(v > 0xFFFFFFFFULL) throw std::out_of_range("instruction value out of 32-bit range");
        return static_cast<uint32_t>(v);
    };

    if(LooksLikeBinary(input)) {
        r.kind_= InputKind::BINARY;
        r.word_= parseword(STRIP_INT_PREFIX(input), 2);
    } else if(LooksLikeHex(input)) {
        r.kind_= InputKind::HEX;
        r.word_= parseword(STRIP_INT_PREFIX(input), 16);
    } else {
        r.kind_= InputKind::ASSEMBLY;
    }
    return r;
}

} // namespace util
