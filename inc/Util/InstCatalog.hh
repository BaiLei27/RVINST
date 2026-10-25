#pragma once

#include <optional>
#include <vector>

#include "ISA/InstFormat.hh"

namespace util {

struct InstCatalog {
    std::string_view name_;
    std::string_view xlen_;
    uint32_t functKey_ {}; // full PACK_KEY; uint16 truncates Zicsr (f3<<8) and many C keys
    uint16_t opcode_ {};
    InstFormat format_ { InstFormat::UNKNOWN };
};

std::vector<InstCatalog> GetAllInstructions(std::optional<InstFormat> filter= std::nullopt);

std::optional<InstFormat> StringToFormat(std::string_view s);

} // namespace util
