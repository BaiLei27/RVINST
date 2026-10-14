
#include <utility>

#include "Util/ErrorHandle.hh"

namespace util {

Error::Error(EC code, std::string msg) noexcept: Code_(code), Msg_(std::move(msg)) { }

EC Error::Code() const noexcept { return Code_; }

std::string_view Error::Msg() const noexcept { return Msg_; }

const char *Error::cStr() const noexcept { return Msg_.c_str(); } // NOLINT

std::unexpected<Error> Unexpected(Error err) { return std::unexpected(std::move(err)); }

} // namespace util
