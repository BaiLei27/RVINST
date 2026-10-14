
#include <utility>

#include "Util/ErrorHandle.hh"

namespace util {

Error::Error(EC code, std::string msg) noexcept: Code_(code), Msg_(std::move(msg)) { }

EC Error::GetCode() const noexcept { return Code_; }

std::string_view Error::Msg() const noexcept { return Msg_; }

const char *Error::cStr() const noexcept { return Msg_.c_str(); } // NOLINT

Error Error::InvalidArg(std::string msg) { return { EC::INVALID_ARGUMENT, std::move(msg) }; }

Error Error::OutOfRange(std::string msg) { return { EC::OUT_OF_RANGE, std::move(msg) }; }

Error Error::DecodeFailed(std::string msg) { return { EC::DECODE_FAILED, std::move(msg) }; }

Error Error::Unsupported(std::string msg) { return { EC::UNSUPPORTED, std::move(msg) }; }

std::unexpected<Error> Unexpected(Error err) { return std::unexpected(std::move(err)); }

} // namespace util
