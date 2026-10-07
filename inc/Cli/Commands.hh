#pragma once

#include <optional>
#include <string_view>

#include "Cli/OutputTable.hh"
#include "ISA/InstFormat.hh"

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
    int Decode(std::string_view input);
    int Encode(std::string_view input);
    int Repl();
    int Reg(std::string_view query);
    int List(std::optional<InstFormat> filter= std::nullopt);

private:
    int processInstruction(uint32_t hexVal);
    int processInstruction(std::string_view assembly);
    int replLoop();

    static void replBanner();
    static void replHelp();
};

} // namespace cli
