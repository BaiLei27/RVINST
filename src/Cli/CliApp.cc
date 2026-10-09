#include "Cli/CliApp.hh"

#include <algorithm>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Cli/Commands.hh"
#include "Cli/InstrEntry.hh"
#include "Cli/OutputTable.hh"
#include "ISA/InstFormat.hh"
#include "config.hpp"

namespace cli {

int CliApp::Run(int argc, char *pArgv[])
{
    bool hasABI   = false;
    bool forceJson= false;
    std::string subcommand;
    std::vector<std::string> remaining;

    const std::span<char *> ARGS(pArgv, static_cast<size_t>(argc));
    for(auto *pArg: ARGS.subspan(std::min<size_t>(1, ARGS.size()))) {
        std::string arg(pArg);

        if(arg == "--abi") {
            hasABI= true;
            continue;
        }
        if(arg == "--no-abi") {
            hasABI= false;
            continue;
        }
        if(arg == "--json") {
            forceJson= true;
            continue;
        }
        if(arg == "-h" || arg == "--help") {
            OutputFormatter fmt(OutputMode::PLAIN);
            fmt.PrintHelp();
            return 0;
        }
        if(arg == "-v" || arg == "--version") {
            std::println("Version:  {}", util::info::G_PROJECT_VERSION);
            std::println("Commit:   {}", util::info::G_COMMIT_HASH);
            std::println("URL:      {}", util::info::G_HOMEPAGE_URL);
            std::println("Copyright:{}", util::info::G_COPYRIGHT);
            return 0;
        }

        if(subcommand.empty()) {
            subcommand= arg;
        } else {
            remaining.emplace_back(arg);
        }
    }

    if(subcommand.empty()) {
        OutputFormatter fmt(detectOutputMode(false));
        fmt.PrintHelp();
        return 0;
    }

    OutputMode mode= detectOutputMode(forceJson);
    Commands cmd({ .hasABI_= hasABI, .outputMode_= mode });

    auto fail= [&mode](std::string_view msg) {
        OutputFormatter fmt(mode);
        fmt.PrintError(msg);
        return 1;
    };

    if(subcommand == "decode") {
        if(remaining.empty()) {
            return fail("decode requires an input argument");
        }
        return cmd.Decode(remaining[0]);
    }

    if(subcommand == "encode") {
        if(remaining.empty()) {
            return fail("encode requires an assembly input");
        }
        // Join remaining args - assembly may contain spaces if unquoted
        std::string input= remaining[0];
        for(size_t i= 1; i < remaining.size(); ++i) {
            input+= " " + remaining[i];
        }
        return cmd.Encode(input);
    }

    if(subcommand == "repl") {
        return cmd.Repl();
    }

    if(subcommand == "reg") {
        if(remaining.empty()) {
            return fail("reg requires a register name or index");
        }
        return cmd.Reg(remaining[0]);
    }

    if(subcommand == "list") {
        std::optional<InstFormat> filter;
        for(size_t i= 0; i < remaining.size(); ++i) {
            if(remaining[i] == "--type" && i + 1 < remaining.size()) {
                filter= stringToFormat(remaining[++i]);
                if(!filter) return fail(std::format("invalid type: {}", remaining[i]));

            } else if(remaining[i].starts_with("--type=")) {
                filter= stringToFormat(remaining[i].substr(7));
                if(!filter) return fail(std::format("invalid type: {}", remaining[i].substr(7)));
            }
        }
        return cmd.List(filter);
    }

    return fail(std::format("unknown command: {} (try --help)", subcommand));
}

} // namespace cli
