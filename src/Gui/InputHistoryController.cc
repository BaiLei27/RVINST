#include "Gui/InputHistoryController.hh"

InputHistoryController::InputHistoryController(size_t maxEntries): MaxEntries_(maxEntries) { }

bool InputHistoryController::IsEmpty() const noexcept { return Entries_.empty(); }

size_t InputHistoryController::Size() const noexcept { return Entries_.size(); }

std::string_view InputHistoryController::At(size_t index) const noexcept
{
    if(index >= Entries_.size()) return {};

    return Entries_[index];
}

const std::deque<std::string> &InputHistoryController::GetEntries() const noexcept
{
    return Entries_;
}

void InputHistoryController::Push(std::string_view text)
{
    if(text.empty()) return;

    std::erase(Entries_, std::string(text));
    Entries_.emplace_front(text);

    while(Entries_.size() > MaxEntries_) {
        Entries_.pop_back();
    }
    CancelBrowse();
}

void InputHistoryController::Clear()
{
    Entries_.clear();
    CancelBrowse();
}

bool InputHistoryController::IsBrowsing() const noexcept
{
    return Cursor_.has_value();
}

size_t InputHistoryController::CursorIndex() const noexcept
{
    return Cursor_.value_or(0);
}

std::optional<std::string_view> InputHistoryController::Step(int delta)
{
    if(Entries_.empty()) return std::nullopt;

    if(!Cursor_) {
        if(delta < 0) return std::nullopt; // down with no cursor: no-op
        Cursor_= 0;
    } else {
        if(delta < 0) {
            if(*Cursor_ == 0) {
                Cursor_.reset();
                return Draft_;
            }
            *Cursor_-= 1;
        } else {
            *Cursor_+= delta;
        }
    }

    if(*Cursor_ >= Entries_.size()) {
        *Cursor_= Entries_.size() - 1;
    }
    return { Entries_[*Cursor_] };
}

void InputHistoryController::JumpTo(size_t index)
{
    if(index >= Entries_.size()) return;
    Cursor_= index;
}

void InputHistoryController::CancelBrowse()
{
    Cursor_.reset();
    Draft_.clear();
}

void InputHistoryController::SetDraft(std::string draft)
{
    Draft_= std::move(draft);
}
