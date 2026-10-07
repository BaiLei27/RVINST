#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

class BinaryFieldWidget;
class AsmMnemonicWidget;

namespace InstCommST { // NOLINT

struct TransparentStringHash {
    using is_transparent= void; // ! Must be set to 'void', enable heterogeneous lookup //NOLINT

    size_t operator() (std::string_view s) const noexcept
    {
        return std::hash<std::string_view> {}(s);
    }

    size_t operator() (const std::string &s) const noexcept
    {
        return std::hash<std::string_view> {}(s);
    }
};

template <class W>
using WidgetMap_u= std::unordered_map<std::string, W *, TransparentStringHash, std::equal_to<>>;

using BinaryFieldWidgetMap_u= WidgetMap_u<BinaryFieldWidget>;
using AsmMnemonicWidgetMap_u= WidgetMap_u<AsmMnemonicWidget>;

} // namespace InstCommST
