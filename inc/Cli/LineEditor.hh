#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "Util/InputHistoryMgr.hh"

// MSYS2 clang64/ucrt64/mingw and MSVC are native Win32; only MSYS/Cygwin
// runtime and Linux/macOS expose POSIX tty APIs.
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MSYS__)
    #define RVINST_NATIVE_WIN 1
#endif

namespace cli {

enum class Key : uint8_t {
    PRINTABLE,   // displayable char, in GetKeyChar()
    ENTER,       // accept the line
    BACKSPACE,   // delete char before caret
    UP,          // history backward
    DOWN,        // history forward
    LEFT,        // move caret left
    RIGHT,       // move caret right
    END_OF_FILE, // Ctrl-D / EOF
};

/// Terminal I/O abstraction so tests can drive the editor with scripted keys.
class IKeySource {
public:
    virtual ~IKeySource()= default;

public:
    [[nodiscard]] virtual Key Peek()                      = 0; // skips noise, returns a valid Key
    [[nodiscard]] virtual char GetKeyChar() const noexcept= 0; // valid when Peek() == PRINTABLE
    [[nodiscard]] virtual bool IsInteractive() const      = 0;
    virtual void WriteString(std::string_view s)          = 0; // paint / move caret
    virtual void EnterRawMode()                           = 0;
    virtual void LeaveRawMode()                           = 0;
};

/// Production backend. Platform state is hidden behind pimpl so the header
/// stays free of <windows.h> / <termios.h>.
class TerminalKeySource final: public IKeySource {
private:
    struct Impl;
    std::unique_ptr<Impl> Impl_;
    char KeyChar_ { '\0' };

public:
    TerminalKeySource();
    ~TerminalKeySource() override;

    TerminalKeySource(const TerminalKeySource &)            = delete;
    TerminalKeySource &operator= (const TerminalKeySource &)= delete;
    TerminalKeySource(TerminalKeySource &&)                 = delete;
    TerminalKeySource &operator= (TerminalKeySource &&)     = delete;

    [[nodiscard]] bool IsInteractive() const override;
    [[nodiscard]] Key Peek() override;
    [[nodiscard]] char GetKeyChar() const noexcept override;
    void EnterRawMode() override;
    void LeaveRawMode() override;
    void WriteString(std::string_view s) override;

private:
    /// nullopt = noise to discard; Key otherwise.
    [[nodiscard]] std::optional<Key> readKeyOnce();
};

/// Single-line editor with caret support for the CLI REPL.
class LineEditor {
private:
    std::string Line_;
    IKeySource &Src_;
    size_t Caret_ {};

public:
    explicit LineEditor(IKeySource &source);
    LineEditor(const LineEditor &)            = delete;
    LineEditor &operator= (const LineEditor &)= delete;
    LineEditor(LineEditor &&)                 = delete;
    LineEditor &operator= (LineEditor &&)     = delete;

public:
    /// nullopt on EOF/Ctrl-D. Piped stdin falls back to std::getline.
    std::optional<std::string> ReadLine(util::InputHistoryMgr &history);
    std::string_view Line() const noexcept;
    size_t Caret() const noexcept;

private:
    void redrawLine();
    void insert(char ch);
    void eraseBefore();
    void moveLeft();
    void moveRight();
    void browseHistory(util::InputHistoryMgr &history, int delta);
};

} // namespace cli
