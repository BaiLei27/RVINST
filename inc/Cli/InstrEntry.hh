#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "ISA/InstFormat.hh"

namespace cli {

struct InstrEntry {
    std::string_view name_;
    std::string_view xlen_;
    uint16_t functKey_ {};
    uint16_t opcode_ {};
    InstFormat format_ { InstFormat::UNKNOWN };
};

std::vector<InstrEntry> getAllInstructions(std::optional<InstFormat> filter= std::nullopt);

std::optional<InstFormat> stringToFormat(std::string_view s);

} // namespace cli
