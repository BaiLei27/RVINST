#include <iostream>
#include <print>

#include "Cli/LineEditor.hh"

#ifdef RVINST_NATIVE_WIN
    #include <io.h>      // _isatty / _fileno
    #include <windows.h> // console input events
#else
    #include <termios.h> // POSIX raw mode (also available under MSYS2/Cygwin)
    #include <unistd.h>  // isatty
#endif

namespace cli {

namespace {

    /// ESC [ A/B/C/D -> UP/DOWN/RIGHT/LEFT; nullopt for unrecognized escapes.
    [[nodiscard]] constexpr std::optional<Key> ESC_TO_KEY(char b) noexcept
    {
        switch(b) {
        case 'A': return Key::UP;
        case 'B': return Key::DOWN;
        case 'C': return Key::RIGHT;
        case 'D': return Key::LEFT;
        default:  return std::nullopt;
        }
    }

#ifdef RVINST_NATIVE_WIN
    // Win32 raw-mode RAII: disable line input + echo.
    class RawMode {
    public:
        RawMode()
        {
            HANDLE pIn= GetStdHandle(STD_INPUT_HANDLE);
            if(INVALID_HANDLE_VALUE == pIn) return;
            if(0 == GetConsoleMode(pIn, &Saved_)) return;
            SetConsoleMode(pIn, Saved_ & ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
            PIn_= pIn;
        }

        ~RawMode()
        {
            if(INVALID_HANDLE_VALUE != PIn_) SetConsoleMode(PIn_, Saved_);
        }

        RawMode(const RawMode &)            = delete;
        RawMode &operator= (const RawMode &)= delete;

    private:
        HANDLE PIn_ { INVALID_HANDLE_VALUE };
        DWORD Saved_ {};
    };
#else
    // POSIX raw-mode RAII: clear canonical + echo flags.
    class RawMode {
    public:
        RawMode()
        {
            if(0 != tcgetattr(STDIN_FILENO, &Saved_)) return;
            termios raw= Saved_;
            raw.c_lflag&= ~static_cast<tcflag_t>(ICANON | ECHO);
            if(0 != tcsetattr(STDIN_FILENO, TCSANOW, &raw)) return;
            Active_= true;
        }

        ~RawMode()
        {
            if(Active_) tcsetattr(STDIN_FILENO, TCSANOW, &Saved_);
        }

        RawMode(const RawMode &)            = delete;
        RawMode &operator= (const RawMode &)= delete;

    private:
        termios Saved_ {};
        bool Active_ {};
    };
#endif

} // namespace

/// Platform state hidden behind the pimpl so the header never touches a
/// terminal API: holds the raw-mode RAII guard on both platforms.
struct TerminalKeySource::Impl {
    std::optional<RawMode> raw_; // engaged while raw mode is active
};

TerminalKeySource::TerminalKeySource(): Impl_(std::make_unique<Impl>()) { }

TerminalKeySource::~TerminalKeySource()= default;

bool TerminalKeySource::IsInteractive() const
{
    static const bool CACHED= [] {
#ifdef RVINST_NATIVE_WIN
        return 0 != _isatty(_fileno(stdin));
#else
        return 0 != isatty(STDIN_FILENO);
#endif
    }();

    return CACHED;
}

void TerminalKeySource::EnterRawMode() { Impl_->raw_.emplace(); }

void TerminalKeySource::LeaveRawMode() { Impl_->raw_.reset(); }

void TerminalKeySource::WriteString(std::string_view s)
{
    fwrite(s.data(), 1, s.size(), stdout);
    fflush(stdout);
}

Key TerminalKeySource::Peek()
{
    for(;;) {
        if(auto k= readKeyOnce()) return *k; // END_OF_FILE or a real key
        // nullopt: noise (Win32 key-release, stray ESC) -> retry
    }
}

char TerminalKeySource::GetKeyChar() const noexcept { return KeyChar_; }

std::optional<Key> TerminalKeySource::readKeyOnce()
{
#ifdef RVINST_NATIVE_WIN
    INPUT_RECORD rec {};
    DWORD n {};

    if(!ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &n)) return Key::END_OF_FILE; // NOLINT
    if(KEY_EVENT != rec.EventType || !rec.Event.KeyEvent.bKeyDown) return std::nullopt;          // NOLINT

    const auto &key= rec.Event.KeyEvent;

    switch(key.wVirtualKeyCode) {
    case VK_RETURN: return Key::ENTER;
    case VK_BACK:   return Key::BACKSPACE;
    case VK_UP:     return Key::UP;
    case VK_DOWN:   return Key::DOWN;
    case VK_LEFT:   return Key::LEFT;
    case VK_RIGHT:  return Key::RIGHT;
    default:        break;
    }
    const auto CH  = key.uChar.UnicodeChar;
    const bool CTRL= 0 != (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));
    // Console often delivers Ctrl-D as VK='D'(0x44) with UnicodeChar=0 (or EOT=4).
    if(0x04 == CH || (CTRL && 'D' == key.wVirtualKeyCode)) return Key::END_OF_FILE;
    if(0 == CH) return std::nullopt; // non-char key (modifiers, F-keys, ...)
#else
    const int CH= std::getchar();

    if(EOF == CH || '\x04' == CH) return Key::END_OF_FILE;
    if('\n' == CH || '\r' == CH) return Key::ENTER;
    if('\x7f' == CH || '\b' == CH) return Key::BACKSPACE;
    if('\x1b' == CH) {
        if('[' != std::getchar()) return std::nullopt; // stray ESC
        return ESC_TO_KEY(static_cast<char>(std::getchar()));
    }

#endif
    KeyChar_= static_cast<char>(CH);
    return Key::PRINTABLE;
}

LineEditor::LineEditor(IKeySource &source): Src_(source) { }

std::optional<std::string> LineEditor::ReadLine(util::InputHistoryMgr &history)
{
    if(!Src_.IsInteractive()) { // piped input: plain getline, no caret editing
        std::string line;
        if(!std::getline(std::cin, line)) return std::nullopt;
        return line;
    }

    Line_.clear();
    Caret_= 0;
    Src_.EnterRawMode();
    redrawLine();

    for(;;) {
        switch(Src_.Peek()) {
        case Key::ENTER: {
            std::println();
            Src_.LeaveRawMode();
            return Line_;
        }
        case Key::END_OF_FILE: {
            std::println();
            Src_.LeaveRawMode();
            return std::nullopt;
        }
        case Key::PRINTABLE: insert(Src_.GetKeyChar()); break;
        case Key::BACKSPACE: eraseBefore(); break;
        case Key::UP:        browseHistory(history, +1); break;
        case Key::DOWN:      browseHistory(history, -1); break;
        case Key::LEFT:      moveLeft(); break;
        case Key::RIGHT:     moveRight(); break;
        }
    }
}

std::string_view LineEditor::Line() const noexcept { return Line_; }

size_t LineEditor::Caret() const noexcept { return Caret_; }

void LineEditor::redrawLine()
{
    std::string out;
    out.reserve(Line_.size() + 32);
    out.append("\r\x1b[2Krvinst> ");
    out.append(Line_);
    const size_t BACK= Line_.size() - std::min(Caret_, Line_.size());
    if(0 != BACK) {
        out.append("\x1b[");
        char buf[16];
        const auto [PTR, EC]= std::to_chars(buf, buf + sizeof(buf), BACK); // NOLINT
        if(std::errc {} == EC) out.append(buf, static_cast<size_t>(PTR - buf));
        out.push_back('D');
    }
    Src_.WriteString(out);
}

void LineEditor::insert(char ch)
{
    if(Caret_ == Line_.size()) { // append: paint one glyph, skip full redraw
        Line_.push_back(ch);
        ++Caret_;
        Src_.WriteString(std::string_view { &ch, 1 });
        return;
    }
    Line_.insert(Line_.begin() + static_cast<std::ptrdiff_t>(Caret_), ch);
    ++Caret_;
    redrawLine();
}

void LineEditor::eraseBefore()
{
    if(0 == Caret_ || Line_.empty()) return;
    Line_.erase(Caret_ - 1, 1);
    --Caret_;
    redrawLine();
}

void LineEditor::moveLeft()
{
    if(0 == Caret_) return;
    --Caret_;
    Src_.WriteString("\x1b[D");
}

void LineEditor::moveRight()
{
    if(Caret_ >= Line_.size()) return;
    ++Caret_;
    Src_.WriteString("\x1b[C");
}

void LineEditor::browseHistory(util::InputHistoryMgr &history, int delta)
{
    if(history.IsEmpty()) return;
    if(0 < delta && !history.IsBrowsing()) history.SetDraft(Line_);
    if(auto next= history.Step(delta)) {
        Line_ = *next; // string_view -> string assign, no temporary std::string
        Caret_= Line_.size();
        redrawLine();
    }
}

} // namespace cli
