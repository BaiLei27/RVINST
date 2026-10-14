#pragma once

#include <optional>
#include <string_view>

#include "Cli/OutputTable.hh"
#include "ISA/InstFormat.hh"
#include "Util/ErrorHandle.hh"

namespace cli {

struct GlobalOptions {
    bool hasABI_ { false };
    OutputMode outputMode_ { OutputMode::PLAIN };
};

class Commands {
private:
    GlobalOptions Opts_;
    OutputFormatter Fmt_;

public:
    explicit Commands(const GlobalOptions &opts);

public:
    [[nodiscard]] util::Expected_u<void> Decode(std::string_view input);
    [[nodiscard]] util::Expected_u<void> Encode(std::string_view input);
    int Repl();
    [[nodiscard]] util::Expected_u<void> Reg(std::string_view query);
    [[nodiscard]] util::Expected_u<void> List(std::optional<InstFormat> filter= std::nullopt);

private:
    [[nodiscard]] util::Expected_u<void> processInstruction(uint32_t hexVal);
    [[nodiscard]] util::Expected_u<void> processInstruction(std::string_view assembly);
    int replLoop();

    static void replBanner();
    static void replHelp();
};

} // namespace cli
