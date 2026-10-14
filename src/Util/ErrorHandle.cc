
#include <utility>

#include "Util/ErrorHandle.hh"

namespace util {

Error::Error(EC code, std::string msg) noexcept
    : Code_(code), Msg_(std::move(msg)) { }

EC Error::Code() const noexcept { return Code_; }

std::string_view Error::Msg() const noexcept { return Msg_; }

const char *Error::cStr() const noexcept { return Msg_.c_str(); } // NOLINT

Error Error::Make(EC code, std::string msg) noexcept { return { code, std::move(msg) }; }

Error Error::InvalidArg(std::string msg) { return Make(EC::INVALID_ARGUMENT, std::move(msg)); }

Error Error::OutOfRange(std::string msg) { return Make(EC::OUT_OF_RANGE, std::move(msg)); }

Error Error::DecodeFailed(std::string msg) { return Make(EC::DECODE_FAILED, std::move(msg)); }

Error Error::EncodeFailed(std::string msg) { return Make(EC::ENCODE_FAILED, std::move(msg)); }

Error Error::Unsupported(std::string msg) { return Make(EC::UNSUPPORTED, std::move(msg)); }

std::unexpected<Error> Unexpected(Error err) { return std::unexpected(std::move(err)); }

} // namespace util
