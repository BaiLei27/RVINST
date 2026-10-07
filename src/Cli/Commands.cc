#include "Cli/Commands.hh"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iostream>
#include <optional>
#include <print>
#include <string>

#include "Core/Instruction.hh"
#include "ISA/InstFormat.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace cli {

Commands::Commands(const GlobalOptions &opts): Opts_(opts), Fmt_(opts.outputMode_) { }

int Commands::Decode(std::string_view input)
{
    const auto PARSED= util::ClassifyInstInput(input);

    if(util::InputKind::ASSEMBLY == PARSED.kind_) {
        Fmt_.PrintError("decode expects hex or binary input, got assembly. Use 'encode' instead.");
        return 1;
    }

    return processInstruction(PARSED.word_);
}

int Commands::Encode(std::string_view input)
{
    const auto PARSED= util::ClassifyInstInput(input);

    if(PARSED.kind_ != util::InputKind::ASSEMBLY) {
        Fmt_.PrintError("encode expects assembly input, got hex/binary. Use 'decode' instead.");
        return 1;
    }

    return processInstruction(input);
}

int Commands::Reg(std::string_view query)
{
    auto idxOpt= isa::LOOKUP_REG_IDX(query);

    if(!idxOpt) {
        // Try plain numeric parse as fallback
        std::string s(query);
        if(std::ranges::all_of(s, [](unsigned char c) { return std::isdigit(c); })) {
            auto v= std::stoul(s);
            if(v < 32) idxOpt= static_cast<uint16_t>(v);
        }
    }
    if(!idxOpt || *idxOpt >= 32) {
        Fmt_.PrintError(std::format("register '{}' not found", query));
        return 1;
    }
    Fmt_.PrintRegister(*idxOpt);

    return 0;
}

int Commands::List(std::optional<InstFormat> filter)
{
    auto entries= getAllInstructions(filter);
    if(entries.empty()) {
        Fmt_.PrintError("no instructions found for the given filter");
        return 1;
    }
    Fmt_.PrintInstructionList(entries);

    return 0;
}

int Commands::processInstruction(uint32_t hexVal)
{
    try {
        Instruction inst(hexVal, Opts_.hasABI_);

        if(!inst.GetTypePtr()) {
            Fmt_.PrintError(std::format("unsupported instruction: 0x{:08X}", hexVal));
            return 1;
        }

        bool ok= false;
        {
            CoutSuppressor suppress;
            ok= inst.Decode();
        }
        if(!ok) {
            Fmt_.PrintError(std::format("failed to decode instruction: 0x{:08X}", hexVal));
            return 1;
        }
        Fmt_.PrintInstruction(inst, std::format("0x{:08X}", hexVal));

        return 0;

    } catch(const std::exception &e) {
        Fmt_.PrintError(e.what());
        return 1;
    } catch(...) {
        Fmt_.PrintError("unknown error during instruction processing");
        return 1;
    }
}

int Commands::processInstruction(std::string_view assembly)
{
    try {
        Instruction inst(assembly, Opts_.hasABI_);

        if(!inst.GetTypePtr()) {
            Fmt_.PrintError(std::format("unsupported instruction: {}", assembly));
            return 1;
        }

        bool ok= false;
        {
            CoutSuppressor suppress;
            ok= inst.Decode();
        }
        if(!ok) {
            Fmt_.PrintError(std::format("failed to encode instruction: {}", assembly));
            return 1;
        }

        Fmt_.PrintInstruction(inst, assembly);

        return 0;

    } catch(const std::exception &e) {
        Fmt_.PrintError(e.what());
        return 1;
    } catch(...) {
        Fmt_.PrintError("unknown error during instruction processing");
        return 1;
    }
}

void Commands::replBanner()
{
    std::println("\n  rvinst REPL - type :help for commands, :quit to exit");
}

void Commands::replHelp()
{
    std::println(
        "\n  REPL commands:\n"
        "    :help, :h          Show this help\n"
        "    quit, q, exit, :quit, :q :exit     Exit REPL\n"
        "    :abi on|off        Toggle ABI register names\n"
        "    :json on|off       Toggle JSON output\n"
        "    :list [--type T]   List supported instructions\n"
        "\n  Input: hex (0x...), binary (0b...), or assembly (add x1,x2,x8)\n"
        "  Auto-detects input type and decodes/encodes accordingly.\n");
}

int Commands::replLoop()
{
    std::string line;
    auto ifQuitRepl= [](std::string_view trimmed) {
        if(trimmed == "q"
           || trimmed == "quit"
           || trimmed == "exit"
           || trimmed == "end") {
            std::println(" exited.");
            return true;
        }
        return false;
    };
    while(true) {
        std::print("rvinst> ");
        fflush(stdout);

        if(!std::getline(std::cin, line)) {
            std::println();
            break;
        }

        size_t start= line.find_first_not_of(" \t\r\n");
        if(std::string::npos == start) continue;

        size_t end  = line.find_last_not_of(" \t\r\n");
        auto trimmed= std::string_view(line).substr(start, end - start + 1);

        if(trimmed.empty()) continue;

        if(ifQuitRepl(trimmed)) return 0;

        if(trimmed.starts_with(':')) {
            auto cmd            = trimmed.substr(1);
            auto sp             = cmd.find(' ');
            auto verb           = cmd.substr(0, sp);
            std::string_view arg= (sp != std::string_view::npos)
                                    ? cmd.substr(sp + 1)
                                    : "";

            if(ifQuitRepl(verb)) return 0;

            if(verb == "help" || verb == "h") {
                replHelp();
                continue;
            }
            if(verb == "abi") {
                if(arg == "on") {
                    Opts_.hasABI_= true;
                    std::println("  ABI names: on");
                } else if(arg == "off") {
                    Opts_.hasABI_= false;
                    std::println("  ABI names: off");
                } else {
                    std::println("  usage: :abi on|off (currently: {})",
                                 Opts_.hasABI_ ? "on" : "off");
                }
                continue;
            }
            if(verb == "json") {
                if(arg == "on") {
                    Opts_.outputMode_= OutputMode::JSON;
                    Fmt_             = OutputFormatter(OutputMode::JSON);
                    std::println("  JSON output: on");
                } else if(arg == "off") {
                    Opts_.outputMode_= OutputMode::COLORED;
                    Fmt_             = OutputFormatter(OutputMode::COLORED);
                    std::println("  JSON output: off");
                } else {
                    std::println("  usage: :json on|off (currently: {})",
                                 OutputMode::JSON == Opts_.outputMode_ ? "on" : "off");
                }
                continue;
            }
            if(verb == "list") {
                std::optional<InstFormat> filter;
                if(arg.starts_with("--type ")) {
                    filter= stringToFormat(arg.substr(7));
                }
                List(filter);
                continue;
            }

            std::println("  unknown command: :{} (try :help)", verb);
            continue;
        }
        // Auto-detect and process
        auto parsed= util::ClassifyInstInput(trimmed);
        if(util::InputKind::HEX == parsed.kind_ || util::InputKind::BINARY == parsed.kind_) {
            processInstruction(parsed.word_);
        } else {
            processInstruction(trimmed);
        }
    }
    return 0;
}

int Commands::Repl()
{
    replBanner();
    replHelp();
    return replLoop();
}

} // namespace cli
