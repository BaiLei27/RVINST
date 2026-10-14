#pragma once

#include <cstdint>
#include <expected>
#include <string>

namespace util {

enum class EC : uint8_t {
    INVALID_ARGUMENT,
    OUT_OF_RANGE,
    DECODE_FAILED,
    UNSUPPORTED,
};

class Error {
    EC Code_ { EC::INVALID_ARGUMENT };
    std::string Msg_;

public:
    Error(EC code, std::string msg) noexcept;

    [[nodiscard]] EC GetCode() const noexcept;

    [[nodiscard]] std::string_view Msg() const noexcept;

    [[nodiscard]] const char *cStr() const noexcept; // NOLINT

    [[nodiscard]] static Error InvalidArg(std::string msg);
    [[nodiscard]] static Error OutOfRange(std::string msg);
    [[nodiscard]] static Error DecodeFailed(std::string msg);
    [[nodiscard]] static Error Unsupported(std::string msg);
};

template <class T>
using Expected_u= std::expected<T, Error>;

[[nodiscard]] std::unexpected<Error> Unexpected(Error err);

} // namespace util

// date:26-10-11-23:12
