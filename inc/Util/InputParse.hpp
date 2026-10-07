#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

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
    if(s.starts_with("0x")
       || s.starts_with("0X")
       || s.starts_with("0b")
       || s.starts_with("0B")) return s.substr(2);

    return s;
}

[[nodiscard]] inline bool LooksLikeHex(std::string_view s)
{
    if(s.empty()) return false;

    auto digits= s.starts_with("0x") || s.starts_with("0X") ? s.substr(2) : s;

    if(digits.empty()) return false;

    return std::ranges::all_of(digits, [](unsigned char c) { return std::isxdigit(c) != 0; });
}

[[nodiscard]] inline bool LooksLikeBinary(std::string_view s)
{
    if(s.empty()) return false;

    std::string_view digits(s);
    if(s.starts_with("0b") || s.starts_with("0B")) {
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
    // NOLINTBEGIN
    auto parseW= [](std::string_view digits, int base) {
        uint32_t value {};
        auto [ptr, ec]= std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
        if(ec != std::errc() || ptr != digits.data() + digits.size()) {
            throw std::out_of_range("instruction value out of 32-bit range");
        }
        return value;
    };
    // NOLINTEND
    if(LooksLikeBinary(input)) {
        r.kind_= InputKind::BINARY;
        r.word_= parseW(STRIP_INT_PREFIX(input), 2);
    } else if(LooksLikeHex(input)) {
        r.kind_= InputKind::HEX;
        r.word_= parseW(STRIP_INT_PREFIX(input), 16);
    } else {
        r.kind_= InputKind::ASSEMBLY;
    }
    return r;
}

/**
 * @brief Parse a C/C++ integer literal (decimal, 0x hex, 0b binary, leading-0 octal, optional +/- sign)
 *  via std::from_chars. Returns false on invalid or out-of-range input.
 * @tparam T
 * @param s
 * @param out
 */
template <typename T>
[[nodiscard]] bool ParseInt(std::string_view s, T &out)
{
    bool neg= false;
    if(!s.empty() && (s.front() == '-' || s.front() == '+')) {
        neg= s.front() == '-';
        s.remove_prefix(1);
    }

    int base= 10;
    if(s.starts_with("0x") || s.starts_with("0X")) {
        base= 16;
        s.remove_prefix(2);
    } else if(s.starts_with("0b") || s.starts_with("0B")) {
        base= 2;
        s.remove_prefix(2);
    } else if(s.starts_with('0')) {
        base= 8;
        s.remove_prefix(1);
    }
    // NOLINTBEGIN
    std::make_unsigned_t<T> mag {};
    auto [ptr, ec]= std::from_chars(s.data(), s.data() + s.size(), mag, base);
    if(ec != std::errc() || ptr != s.data() + s.size()) return false;

    if constexpr(std::is_signed_v<T>) {
        using U        = std::make_unsigned_t<T>;
        constexpr U MAX= static_cast<U>(std::numeric_limits<T>::max());
        if(neg) {
            if(mag > MAX + 1U) return false;
            out= (mag == MAX + 1U) ? std::numeric_limits<T>::min() : static_cast<T>(-static_cast<T>(mag));
        } else {
            if(mag > MAX) return false;
            out= static_cast<T>(mag);
        }
    } else {
        if(neg) return false;
        out= static_cast<T>(mag);
    }
    // NOLINTEND

    return true;
}

} // namespace util
