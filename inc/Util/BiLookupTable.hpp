#pragma once

#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <print>
#include <string_view>
#include <flat_map>

template <typename KeyT= uint16_t>
class BiLookupTable { // Bidirectional lookup table for code2name and name2code
public:
    struct NameInfo {
        std::string manual_;
        std::string_view XLEN_; // NOLINT
        std::string_view name_;
    };

    struct IndexInfo {
        std::string manual_;
        std::string_view XLEN_; // NOLINT
        KeyT keyID_ {};
        uint16_t opcode_ {};
    };

    // std::flat_map keeps keys sorted in contiguous std::vector-backed
    // storage: cache-friendly reads and O(log n) binary-search lookup.
    using intMapName_u = std::flat_map<KeyT, NameInfo>;
    using strMapIndex_u= std::flat_map<std::string_view, IndexInfo>;

    BiLookupTable(const intMapName_u &code2info,
                  const strMapIndex_u &name2info)
        : Code2NameInfo_(code2info),
          Name2IdxInfo_(name2info) { }

    BiLookupTable(const intMapName_u &&code2info,
                  const strMapIndex_u &&name2info)
        : Code2NameInfo_(std::move(code2info)),
          Name2IdxInfo_(std::move(name2info)) { }

    BiLookupTable(const BiLookupTable &)            = delete;
    BiLookupTable &operator= (const BiLookupTable &)= delete;
    BiLookupTable(BiLookupTable &&)                 = default;
    BiLookupTable &operator= (BiLookupTable &&)     = default;

public:
    std::optional<NameInfo> Find(KeyT code) const noexcept
    {
        auto it= Code2NameInfo_.find(code);
        return it != Code2NameInfo_.end() ? std::make_optional(it->second) : std::nullopt;
    }

    std::optional<IndexInfo> Find(std::string_view name) const noexcept
    {
        auto it= Name2IdxInfo_.find(name);
        return it != Name2IdxInfo_.end() ? std::make_optional(it->second) : std::nullopt;
    }

    bool Contains(KeyT code) const noexcept { return Code2NameInfo_.contains(code); }

    bool Contains(std::string_view name) const noexcept { return Name2IdxInfo_.contains(name); }

    void PrintCode2NameMap() const
    {
        std::string buf;
        buf.reserve(128 + Code2NameInfo_.size() * 192);
        auto out= std::back_inserter(buf);
        std::format_to(out, "==================================== Code -> Name Info Map ====================================\n");
        if(Code2NameInfo_.empty()) {
            std::format_to(out, "Map is empty.\n");
            std::print("{}", buf);
            return;
        }

        for(const auto &[key, name2info]: Code2NameInfo_) {
            std::format_to(out,
                           "--------------------------------------------------------------------------------------------\n"
                           "  Code (functKey): 0x{:04X}\n"
                           "  Instruction Name: {}\n"
                           "  XLEN Architecture: {}\n"
                           "  Manual URL: {}\n",
                           key,
                           name2info.name_,
                           name2info.XLEN_,
                           name2info.manual_);
        }
        std::format_to(out, "==============================================================================================\n\n");
        std::print("{}", buf);
    }

    void PrintName2IndexMap() const
    {
        std::string buf;
        buf.reserve(128 + Name2IdxInfo_.size() * 192);
        auto out= std::back_inserter(buf);
        std::format_to(out, "==================================== Name -> Index Info Map ===================================\n");
        if(Name2IdxInfo_.empty()) {
            std::format_to(out, "Map is empty.\n");
            std::print("{}", buf);
            return;
        }

        for(const auto &[name, index2info]: Name2IdxInfo_) {
            std::format_to(out,
                           "--------------------------------------------------------------------------------------------\n"
                           "  Instruction Name: {}\n"
                           "  XLEN Architecture: {}\n"
                           "  KeyID: 0x{:04X}\n"
                           "  Opcode: 0x{:02X}\n"
                           "  Manual URL: {}\n",
                           name,
                           index2info.XLEN_,
                           index2info.keyID_,
                           index2info.opcode_,
                           index2info.manual_);
        }
        std::format_to(out, "==============================================================================================\n\n");
        std::print("{}", buf);
    }

private:
    strMapIndex_u Name2IdxInfo_;
    intMapName_u Code2NameInfo_;
};

// Date:25/12/21/12:49
