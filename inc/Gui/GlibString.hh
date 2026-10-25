#pragma once

#include <string_view>
#include <glibmm/ustring.h>

namespace gui {

// string_view → Glib::ustring (NUL-free ISA / UI strings).
[[nodiscard]] inline Glib::ustring ToGlibUstring(std::string_view sv)
{
    return { sv.data(), sv.size() };
}

} // namespace gui

// date:26-10-25-22:46
