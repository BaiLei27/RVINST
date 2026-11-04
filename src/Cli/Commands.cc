

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"
#include "Cli/Commands.hh"
#include "Cli/LineEditor.hh"

namespace cli {

Commands::Commands(const GlobalOptions &opts, bool saveHistory)
    : Opts_(opts), Fmt_(opts.outputMode_), SaveHistory_(saveHistory) { }

util::Expected_u<void> Commands::processInstruction(InstInput auto input)
{
    using Raw_u           = std::remove_cvref_t<decltype(input)>;
    constexpr bool IS_WORD= std::same_as<Raw_u, uint32_t>;

    Instruction inst(input, Opts_.hasABI_);

    auto label= [&] {
        if constexpr(IS_WORD) {
            return std::format("{:#010x}", input);
        } else {
            return std::string(input);
        }
    }();

    if(!inst.TypePtr()) {
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
    auto idxOpt= isa::LookupRegIdx(query);

    if(!idxOpt) { // try parsing as a number
        std::string s(query);
        uint16_t v {};
        const char *end     = s.data() + s.size(); // NOLINT
        const auto [PTR, EC]= std::from_chars(s.data(), end, v, 10);
        if(std::errc() == EC && PTR == end && v < 32) idxOpt= v;
    }

    if(!idxOpt || *idxOpt >= 32) {
        return util::Unexpected(util::Error::InvalidArg(std::format("register '{}' not found", query)));
    }
    Fmt_.PrintRegister(*idxOpt);

    return {};
}

util::Expected_u<void> Commands::List(std::optional<InstFormat> filter)
{
    auto entries= util::GetAllInstructions(filter);
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
        "    :hist              Show input history (most recent first)\n"
        "    :file <path>       Process a file line by line into the history\n"
        "\n  Input: hex (0x...), binary (0b...), or assembly (add x1,x2,x8)\n"
        "  Auto-detects input type and decodes/encodes accordingly.\n");
}

void Commands::replHistory()
{
    if(History_.IsEmpty()) {
        std::println("  (history is empty)");
        return;
    }
    std::println("  Input history ({} entries, most recent first):", History_.Size());

    for(size_t idx= 0; const auto &entry: History_.Entries()) {
        std::println("    {:>3}: {}", idx++, entry);
    }
}

util::Expected_u<void> Commands::ProcessFile(std::string_view path)
{
    return util::FileIo::ForEachLine(path, [this](std::string_view raw) {
        const auto TRIMMED= util::TrimAsciiWs(raw);
        if(TRIMMED.empty()) return;

        History_.Push(TRIMMED); // shared with manual input: browsable + :hist
        auto parsed= util::ClassifyInstInput(TRIMMED);
        if(!parsed) {
            Fmt_.PrintError(std::string(parsed.error().Msg()));
            return;
        }
        auto decoded= util::MakeDecodedInstruction(TRIMMED, *parsed, Opts_.hasABI_);
        if(!decoded) {
            Fmt_.PrintError(std::string(decoded.error().Msg()));
            return;
        }
        Fmt_.PrintInstruction(**decoded, TRIMMED);
    });
}

int Commands::replLoop()
{
    auto source= std::make_unique<TerminalKeySource>(); // platform backend lives in LineEditor.cc
    cli::LineEditor editor(*source);

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
        auto line= editor.ReadLine(History_);
        if(!line) {
            break;
        }

        const auto TRIMMED= util::TrimAsciiWs(*line);
        if(TRIMMED.empty()) continue;

        History_.Push(TRIMMED); // remember the command for Up/Down keys

        if(ifQuitRepl(TRIMMED)) return 0;

        if(TRIMMED.starts_with(':')) {
            auto cmd            = TRIMMED.substr(1);
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
            if(verb == "hist" || verb == "history") {
                replHistory();
                continue;
            }
            if(verb == "file") {
                if(arg.empty()) {
                    std::println("  usage: :file <path>");
                    continue;
                }
                if(auto result= ProcessFile(arg); !result) {
                    Fmt_.PrintError(std::string(result.error().Msg()));
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
                    filter= util::StringToFormat(arg.substr(7));
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
        auto parsed= util::ClassifyInstInput(TRIMMED);
        if(!parsed) {
            Fmt_.PrintError(std::string(parsed.error().Msg()));
            continue;
        }
        util::Expected_u<void> result;
        if(util::InputKind::HEX == parsed->kind_ || util::InputKind::BINARY == parsed->kind_) {
            result= processInstruction(parsed->word_);
        } else {
            result= processInstruction(TRIMMED);
        }
        if(!result) Fmt_.PrintError(std::string(result.error().Msg()));
    }
    return 0;
}

int Commands::Repl()
{
    if(SaveHistory_) {
        if(auto r= FileMgr_.LoadHistory(History_); !r) {
            std::println("  (no history loaded: {})", r.error().Msg());
        } else if(History_.Size() > 0) {
            std::println("  loaded {} history entries", History_.Size());
        }
    }

    replBanner();
    replHelp();
    const int RC= replLoop();

    if(SaveHistory_) {
        if(auto r= FileMgr_.SaveHistory(History_); !r) {
            std::println("  (failed to save history: {})", r.error().Msg());
        }
    }
    return RC;
}

} // namespace cli
