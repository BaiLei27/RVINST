
#include <algorithm>
#include <cctype>

#include "Util/InputParse.hh"

namespace util {

[[nodiscard]] constexpr std::string_view STRIP_INT_PREFIX(std::string_view s) noexcept
{
    if(s.starts_with("0x")
       || s.starts_with("0X")
       || s.starts_with("0b")
       || s.starts_with("0B")) return s.substr(2);

    return s;
}

bool LooksLikeHex(std::string_view s)
{
    if(s.empty()) return false;

    auto digits= s.starts_with("0x") || s.starts_with("0X") ? s.substr(2) : s;

    if(digits.empty()) return false;

    return std::ranges::all_of(digits, [](unsigned char c) { return std::isxdigit(c) != 0; });
}

bool LooksLikeBinary(std::string_view s)
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

Expected_u<ParsedInstInput> ClassifyInstInput(std::string_view input)
{
    ParsedInstInput r;
    // NOLINTBEGIN
    auto parseW= [](std::string_view digits, int base) -> Expected_u<uint32_t> {
        uint32_t value {};
        auto [ptr, ec]= std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
        if(ec != std::errc() || ptr != digits.data() + digits.size()) {
            return Unexpected(Error::OutOfRange("instruction value out of 32-bit range"));
        }
        return value;
    };
    // NOLINTEND
    if(LooksLikeBinary(input)) {
        r.kind_  = InputKind::BINARY;
        auto word= parseW(STRIP_INT_PREFIX(input), 2);
        if(!word) return Unexpected(word.error());
        r.word_= *word;
    } else if(LooksLikeHex(input)) {
        r.kind_  = InputKind::HEX;
        auto word= parseW(STRIP_INT_PREFIX(input), 16);
        if(!word) return Unexpected(word.error());
        r.word_= *word;
    } else {
        r.kind_= InputKind::ASSEMBLY;
    }
    return r;
}

} // namespace util
