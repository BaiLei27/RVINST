#pragma once

#include <charconv>
#include <limits>
#include <type_traits>

#include "Util/ErrorHandle.hh"

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

[[nodiscard]] constexpr std::string_view STRIP_INT_PREFIX(std::string_view s) noexcept;
[[nodiscard]] bool LooksLikeHex(std::string_view s);
[[nodiscard]] bool LooksLikeBinary(std::string_view s);
[[nodiscard]] Expected_u<ParsedInstInput> ClassifyInstInput(std::string_view input);

/**
 * @brief Parse a C/C++ integer literal (decimal, 0x hex, 0b binary, leading-0 octal, optional +/- sign)
 *  via std::from_chars. Returns false on invalid or out-of-range input.
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

// update:26-10-12-00:17
