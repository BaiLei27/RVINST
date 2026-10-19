#include "Util/InputHistoryMgr.hh"

namespace util {

InputHistoryMgr::InputHistoryMgr(size_t maxEntries): MaxEntries_(maxEntries) { }

bool InputHistoryMgr::IsEmpty() const noexcept { return Entries_.empty(); }

size_t InputHistoryMgr::Size() const noexcept { return Entries_.size(); }

std::string_view InputHistoryMgr::At(size_t index) const noexcept
{
    if(index >= Entries_.size()) return {};

    return Entries_[index];
}

const std::deque<std::string> &InputHistoryMgr::GetEntries() const noexcept
{
    return Entries_;
}

void InputHistoryMgr::Push(std::string_view text)
{
    if(text.empty()) return;

    std::erase_if(Entries_, [&](const std::string &e) { return e == text; });
    Entries_.emplace_front(text);

    while(Entries_.size() > MaxEntries_) {
        Entries_.pop_back();
    }
    CancelBrowse();
}

void InputHistoryMgr::Clear()
{
    Entries_.clear();
    CancelBrowse();
}

bool InputHistoryMgr::IsBrowsing() const noexcept
{
    return Cursor_.has_value();
}

size_t InputHistoryMgr::CursorIndex() const noexcept
{
    return Cursor_.value_or(0);
}

std::optional<std::string_view> InputHistoryMgr::Step(int delta)
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

void InputHistoryMgr::JumpTo(size_t index)
{
    if(index >= Entries_.size()) return;
    Cursor_= index;
}

void InputHistoryMgr::CancelBrowse()
{
    Cursor_.reset();
    Draft_.clear();
}

void InputHistoryMgr::SetDraft(std::string draft)
{
    Draft_= std::move(draft);
}

} // namespace util
