
#include <cstdio>
#include <iostream>

// MSYS2 clang64/ucrt64/mingw and MSVC are native Win32 (no termios).
// Only the MSYS/Cygwin runtime and Linux/macOS expose POSIX tty APIs.
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MSYS__)
    #define RVINST_NATIVE_WIN 1
#endif

#if defined(RVINST_NATIVE_WIN)
    #include <io.h>      // _isatty / _fileno
    #include <windows.h> // console input events
#else
    #include <termios.h> // POSIX raw mode (also available under MSYS2/Cygwin)
    #include <unistd.h>  // isatty
#endif

#include "ISA/Regs.hh"
#include "Util/InputParse.hh"
#include "Cli/Commands.hh"

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

    for(size_t idx= 0; const auto &entry: History_.GetEntries()) {
        std::println("    {:>3}: {}", idx++, entry);
    }
}

util::Expected_u<void> Commands::ProcessFile(std::string_view path)
{
    return util::FileIo::ForEachLine(path, [this](std::string_view raw) {
        const auto TRIMMED= util::TRIM_ASCII_WS(raw);
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

namespace {

    void redrawPromptLine(std::string_view line)
    {
        std::print("\r\x1b[2K");
        std::print("rvinst> {}", line);
        fflush(stdout);
    }

    void applyHistoryStep(std::string &line, util::InputHistoryMgr &history, int delta)
    {
        if(0 < delta && !history.IsBrowsing()) history.SetDraft(line);
        if(auto next= history.Step(delta); next) {
            line= *next;
            redrawPromptLine(line);
        }
    }

    void eraseLastChar(std::string &line)
    {
        if(line.empty()) return;
        line.pop_back();
        std::print("\b \b");
    }

    void appendChar(std::string &line, util::InputHistoryMgr &history, char ch)
    {
        if(history.IsBrowsing()) history.CancelBrowse();
        line.push_back(ch);
        std::putchar(ch);
        fflush(stdout);
    }

#ifdef RVINST_NATIVE_WIN
    // Native Win32 (MSVC / MSYS2 clang64|ucrt64|mingw): arrows are VK_* events.
    class ScopedRawMode {
    public:
        ScopedRawMode(): HIn_(GetStdHandle(STD_INPUT_HANDLE))
        {
            if(INVALID_HANDLE_VALUE == HIn_) return;
            if(0 == GetConsoleMode(HIn_, &Saved_)) return;
            SetConsoleMode(HIn_, Saved_ & ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
            Active_= true;
        }

        ~ScopedRawMode()
        {
            if(Active_) SetConsoleMode(HIn_, Saved_);
        }

        ScopedRawMode(const ScopedRawMode &)            = delete;
        ScopedRawMode &operator= (const ScopedRawMode &)= delete;

    private:
        HANDLE HIn_ { INVALID_HANDLE_VALUE };
        DWORD Saved_ { 0 };
        bool Active_ { false };
    };

    bool readLinePlatform(std::string &line, util::InputHistoryMgr &history)
    {
        line.clear();
        while(true) {
            INPUT_RECORD rec {};
            DWORD n= 0;
            if(!ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &n)) return false;
            if(KEY_EVENT != rec.EventType || !rec.Event.KeyEvent.bKeyDown) continue;
            const auto &key= rec.Event.KeyEvent;
            if(VK_RETURN == key.wVirtualKeyCode) {
                std::println();
                return true;
            }
            if(VK_BACK == key.wVirtualKeyCode) {
                eraseLastChar(line);
                continue;
            }
            if(VK_UP == key.wVirtualKeyCode) {
                applyHistoryStep(line, history, +1);
                continue;
            }
            if(VK_DOWN == key.wVirtualKeyCode) {
                applyHistoryStep(line, history, -1);
                continue;
            }
            if(VK_LEFT == key.wVirtualKeyCode || VK_RIGHT == key.wVirtualKeyCode) continue;
            const wchar_t WC= key.uChar.UnicodeChar;
            if(0 == WC) continue;
            appendChar(line, history, static_cast<char>(WC)); // ASCII instruction input
        }
    }

    bool isStdInTty() { return 0 != _isatty(_fileno(stdin)); }
#else
    // POSIX tty (Linux / macOS / MSYS|Cygwin runtime): arrows as ESC [ A / B.
    class ScopedRawMode {
    public:
        ScopedRawMode()
        {
            if(0 != tcgetattr(STDIN_FILENO, &Saved_)) return;
            termios raw= Saved_;
            raw.c_lflag&= ~static_cast<tcflag_t>(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            Active_= true;
        }

        ~ScopedRawMode()
        {
            if(Active_) tcsetattr(STDIN_FILENO, TCSANOW, &Saved_);
        }

        ScopedRawMode(const ScopedRawMode &)            = delete;
        ScopedRawMode &operator= (const ScopedRawMode &)= delete;

    private:
        termios Saved_ {};
        bool Active_ { false };
    };

    bool readLinePlatform(std::string &line, util::InputHistoryMgr &history)
    {
        line.clear();
        while(true) {
            const int CH= std::getchar();
            if(EOF == CH || '\x04' == CH) {
                std::println();
                return false;
            }
            if('\n' == CH || '\r' == CH) {
                std::println();
                return true;
            }
            if('\x7f' == CH || '\b' == CH) {
                eraseLastChar(line);
                continue;
            }
            if('\x1b' == CH) {
                if('[' == std::getchar()) {
                    switch(const int CODE= std::getchar()) {
                    case 'A': applyHistoryStep(line, history, +1); continue;
                    case 'B': applyHistoryStep(line, history, -1); continue;
                    default:  continue;
                    }
                }
                continue;
            }
            appendChar(line, history, static_cast<char>(CH));
        }
    }

    bool isStdInTty() { return 0 != isatty(STDIN_FILENO); }
#endif

    // Read one line with arrow-key history navigation when stdin is an
    // interactive terminal; falls back to plain getline for piped input.
    // Returns false on EOF (empty output means EOF was hit).
    // Shares the same Step semantics as the GUI browseHistory() handler:
    // Up steps forward into older entries, Down steps back toward the draft.
    bool readLineInteractive(std::string &line, util::InputHistoryMgr &history)
    {
        if(!isStdInTty()) return static_cast<bool>(std::getline(std::cin, line));

        ScopedRawMode raw;
        return readLinePlatform(line, history);
    }

} // namespace

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

        if(!readLineInteractive(line, History_)) {
            break;
        }

        const auto TRIMMED= util::TRIM_ASCII_WS(line);
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
