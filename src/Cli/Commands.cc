#include "Cli/Commands.hh"

#include <cstdio>
#include <iostream>

#include "Core/Instruction.hh"
#include "ISA/InstFormat.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hh"

namespace cli {

Commands::Commands(const GlobalOptions &opts): Opts_(opts), Fmt_(opts.outputMode_) { }

template <InstInput Input>
util::Expected_u<void> Commands::processInstruction(Input input)
{
    using Raw_u           = std::remove_cvref_t<Input>;
    constexpr bool IS_WORD= std::same_as<Raw_u, uint32_t>;

    Instruction inst(input, Opts_.hasABI_);

    auto label= [&] {
        if constexpr(IS_WORD) {
            return std::format("{:#010x}", input);
        } else {
            return std::string(input);
        }
    }();

    if(!inst.GetTypePtr()) {
        return util::Unexpected(util::Error::Unsupported(std::format("unsupported instruction: {}", label)));
    }

    util::Expected_u<void> decoded;
    {
        CoutSuppressor suppress;
        decoded= inst.Decode();
    }
    if(!decoded) {
        const auto REASON= decoded.error().Msg();
        if constexpr(IS_WORD) {
            return util::Unexpected(util::Error::DecodeFailed(
                std::format("failed to decode instruction: {} ({})", label, REASON)));
        } else {
            return util::Unexpected(util::Error::EncodeFailed(
                std::format("failed to encode instruction: {} ({})", label, REASON)));
        }
    }

    Fmt_.PrintInstruction(inst, label);
    return {};
}

util::Expected_u<void> Commands::Decode(std::string_view input)
{
    const auto PARSED= util::ClassifyInstInput(input);
    if(!PARSED) return util::Unexpected(PARSED.error());

    if(util::InputKind::ASSEMBLY == PARSED->kind_) {
        return util::Unexpected(util::Error::InvalidArg("decode expects hex or binary input, got assembly. Use 'encode' instead."));
    }

    return processInstruction(PARSED->word_);
}

util::Expected_u<void> Commands::Encode(std::string_view input)
{
    const auto PARSED= util::ClassifyInstInput(input);
    if(!PARSED) return util::Unexpected(PARSED.error());

    if(PARSED->kind_ != util::InputKind::ASSEMBLY) {
        return util::Unexpected(util::Error::InvalidArg("encode expects assembly input, got hex/binary. Use 'decode' instead."));
    }

    return processInstruction(input);
}

util::Expected_u<void> Commands::Reg(std::string_view query)
{
    auto idxOpt= isa::LOOKUP_REG_IDX(query);

    if(!idxOpt) { // try parsing as a number
        std::string s(query);
        uint16_t v {};
        const char *end     = s.data() + s.size(); // NOLINT
        const auto [ptr, ec]= std::from_chars(s.data(), end, v, 10);
        if(std::errc() == ec && ptr == end && v < 32) idxOpt= v;
    }

    if(!idxOpt || *idxOpt >= 32) {
        return util::Unexpected(util::Error::InvalidArg(std::format("register '{}' not found", query)));
    }
    Fmt_.PrintRegister(*idxOpt);

    return {};
}

util::Expected_u<void> Commands::List(std::optional<InstFormat> filter)
{
    auto entries= getAllInstructions(filter);
    if(entries.empty()) {
        return util::Unexpected(util::Error::InvalidArg("no instructions found for the given filter"));
    }
    Fmt_.PrintInstructionList(entries);

    return {};
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
                if(auto r= List(filter); !r) {
                    Fmt_.PrintError(std::string(r.error().Msg()));
                }
                continue;
            }

            std::println("  unknown command: :{} (try :help)", verb);
            continue;
        }
        // Auto-detect and process
        auto parsed= util::ClassifyInstInput(trimmed);
        if(!parsed) {
            Fmt_.PrintError(std::string(parsed.error().Msg()));
            continue;
        }
        util::Expected_u<void> result;
        if(util::InputKind::HEX == parsed->kind_ || util::InputKind::BINARY == parsed->kind_) {
            result= processInstruction(parsed->word_);
        } else {
            result= processInstruction(trimmed);
        }
        if(!result) Fmt_.PrintError(std::string(result.error().Msg()));
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
