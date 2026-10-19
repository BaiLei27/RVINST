#pragma once

#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace util {

/**
 * @brief Model-only input-history manager.
 *
 * Owns the bounded, deduplicated history deque plus the browse cursor state
 * (draft, index). The Gtk::Entry / Popover wiring lives in the window class,
 * and the CLI REPL line reader drives the same model; this class deliberately
 * knows nothing about widgets so it stays unit-testable and UI-framework-free.
 */
class InputHistoryMgr {
private:
    std::string Draft_;
    std::deque<std::string> Entries_;
    std::optional<size_t> Cursor_;
    size_t MaxEntries_;

public:
    explicit InputHistoryMgr(size_t maxEntries);

public:
    bool IsEmpty() const noexcept;
    size_t Size() const noexcept;
    std::string_view At(size_t index) const noexcept;
    const std::deque<std::string> &GetEntries() const noexcept;

    void Push(std::string_view text); // dedupe + move-to-front, cap at MaxEntries_
    void Clear();                     // reset entries and browse state

    bool IsBrowsing() const noexcept;
    size_t CursorIndex() const noexcept;
    /**
     * @brief Start (or continue) browsing from the given delta direction.
     * @return entry text to show, or nullopt to keep the current line
     */
    [[nodiscard]] std::optional<std::string_view> Step(int delta);
    void JumpTo(size_t index);
    void CancelBrowse();
    void SetDraft(std::string draft); // remember text before first browse step
};

} // namespace util
