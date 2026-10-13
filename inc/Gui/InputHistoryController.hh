#pragma once

#include <deque>
#include <optional>
#include <string>
#include <string_view>

/**
 * @brief Model-only input-history controller.
 *
 * Owns the bounded, deduplicated history deque plus the browse cursor state
 * (draft, index). The Gtk::Entry / Popover wiring lives in the window class;
 * this class deliberately knows nothing about widgets so it stays unit-testable
 * and UI-framework-free.
 */
class InputHistoryController {
private:
    std::deque<std::string> Entries_;
    std::optional<size_t> Cursor_;
    std::string Draft_;
    size_t MaxEntries_;

public:
    explicit InputHistoryController(size_t maxEntries);

public:
    [[nodiscard]] bool IsEmpty() const noexcept;
    [[nodiscard]] size_t Size() const noexcept;
    [[nodiscard]] std::string_view At(size_t index) const noexcept;
    [[nodiscard]] const std::deque<std::string> &GetEntries() const noexcept;

    void Push(std::string_view text); // dedupe + move-to-front, cap at MaxEntries_
    void Clear();                     // reset entries and browse state

    [[nodiscard]] bool IsBrowsing() const noexcept;
    [[nodiscard]] size_t CursorIndex() const noexcept;
    /**
     * @brief  Start (or continue) browsing from the given delta direction.
     * @param delta
     * @return std::optional<std::string_view> the entry text that should be shown, or nullopt to keep current.
     */
    [[nodiscard]] std::optional<std::string_view> Step(int delta);
    /// Select the entry at index directly (applies it to the entry box).
    void JumpTo(size_t index);
    void CancelBrowse();
    void SetDraft(std::string draft); // remember text before first browse step
};

// Date:26/10/12
