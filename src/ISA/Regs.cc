#include <algorithm>
#include <cctype>
#include <charconv>
#include <flat_map>
#include <initializer_list>
#include <string>
#include <utility>

#include "ISA/Regs.hh"

namespace isa {
namespace {

    using AbiMap_u= std::flat_map<std::string_view, uint16_t>;

    [[nodiscard]] constexpr std::optional<strPair_u> PARSE_REGFILE_NAME(const RegFileTable_u &table,
                                                                        size_t idx) noexcept
    {
        if(idx >= table.size()) return std::nullopt;
        return table[idx];
    }

    [[nodiscard]] constexpr std::string_view LOOKUP_REGFILE_NAME(const RegFileTable_u &table,
                                                                 size_t regIdx,
                                                                 bool hasSetABI,
                                                                 std::string_view fallback) noexcept
    {
        if(regIdx >= table.size()) return fallback;
        const auto &e= table[regIdx];
        return hasSetABI ? e.second : e.first;
    }

    [[nodiscard]] bool AllOfCtype(std::string_view target, int (*pPred)(int)) noexcept
    {
        return std::ranges::all_of(target, [pPred](unsigned char c) { return 0 != pPred(c); });
    }

    [[nodiscard]] std::optional<uint16_t> ParseU16(std::string_view target, int base) noexcept
    {
        uint16_t result {};
        const auto *pBegin= target.data();
        const auto *pEnd  = pBegin + target.size(); // NOLINT
        auto [pParsed, ec]= std::from_chars(pBegin, pEnd, result, base);
        if(std::errc {} == ec && pParsed == pEnd) {
            return result;
        }
        return std::nullopt;
    }

    void AsciiLowerInPlace(std::string &s) noexcept
    {
        std::ranges::transform(s, s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    }

    [[nodiscard]] AbiMap_u MakeAbiMap(const RegFileTable_u &table,
                                      std::initializer_list<std::pair<std::string_view, uint16_t>> extras= {})
    {
        AbiMap_u m;
        for(uint16_t i= 0; i < G_REG_NUMBER; ++i) {
            m.emplace(table[i].second, i);
        }
        for(const auto &[k, v]: extras) {
            m.emplace(k, v);
        }
        return m;
    }

    [[nodiscard]] std::optional<uint16_t> FindAbi(const AbiMap_u &map, std::string_view name) noexcept
    {
        if(const auto IT= map.find(name); IT != map.end()) return IT->second;
        return std::nullopt;
    }

    /// "x12" / "f3" style: prefix + decimal index in [0, G_REG_NUMBER).
    [[nodiscard]] std::optional<uint16_t> LookupPrefixedIndex(std::string_view lower, char prefix) noexcept
    {
        if(!lower.starts_with(prefix) || !AllOfCtype(lower.substr(1), std::isdigit)) {
            return std::nullopt;
        }
        if(auto res= ParseU16(lower.substr(1), 10); res && *res < G_REG_NUMBER) {
            return res;
        }
        return std::nullopt;
    }

    /// ABI name -> index; built once. "s0"/"fp" alias x8 (table stores "s0/fp").
    [[nodiscard]] const AbiMap_u &AbiName2RegIdx()
    {
        static const AbiMap_u S_MAP= MakeAbiMap(G_INDEX_REGS_ABI,
                                                {
                                                    { "s0", 8 },
                                                    { "fp", 8 }
        });

        return S_MAP;
    }

    [[nodiscard]] const AbiMap_u &AbiName2FregIdx()
    {
        static const AbiMap_u S_MAP= MakeAbiMap(G_INDEX_FREGS_ABI);
        return S_MAP;
    }

    [[nodiscard]] const AbiMap_u &CsrName2Num()
    {
        static const auto S_MAP= [] {
            AbiMap_u m;
            for(const auto &e: G_UCSR_TABLE) {
                m.emplace(e.name_, e.num_);
            }
            return m;
        }();
        return S_MAP;
    }

    consteval bool CSR_TABLE_SORTED_BY_NUM() noexcept
    {
        for(size_t i= 1; i < G_UCSR_TABLE.size(); ++i) {
            if(G_UCSR_TABLE[i].num_ <= G_UCSR_TABLE[i - 1].num_) return false;
        }
        return true;
    }

    static_assert(CSR_TABLE_SORTED_BY_NUM());

} // namespace

// Not constexpr: cross-TU APIs cannot keep constexpr (implicitly inline) with
// out-of-line definitions; call sites only need runtime lookup.
std::optional<strPair_u> ParseRegName(size_t idx) noexcept
{
    return PARSE_REGFILE_NAME(G_INDEX_REGS_ABI, idx);
}

std::string_view LookupRegName(size_t regIdx, bool hasSetABI, std::string_view fallback) noexcept
{
    return LOOKUP_REGFILE_NAME(G_INDEX_REGS_ABI, regIdx, hasSetABI, fallback);
}

std::optional<strPair_u> ParseFregName(size_t idx) noexcept
{
    return PARSE_REGFILE_NAME(G_INDEX_FREGS_ABI, idx);
}

std::string_view LookupFregName(size_t regIdx, bool hasSetABI, std::string_view fallback) noexcept
{
    return LOOKUP_REGFILE_NAME(G_INDEX_FREGS_ABI, regIdx, hasSetABI, fallback);
}

std::optional<std::string_view> LookupCSRName(uint16_t num) noexcept
{
    const auto *const IT= std::ranges::lower_bound(G_UCSR_TABLE, num, {}, &CSREntry::num_);
    if(IT != G_UCSR_TABLE.end() && IT->num_ == num) return IT->name_;
    return std::nullopt;
}

std::optional<uint16_t> LookupRegIdx(std::string_view target)
{
    std::string lower(target);
    AsciiLowerInPlace(lower);

    if(auto idx= LookupPrefixedIndex(lower, 'x')) return idx;

    // Integer / hex forms are GPR-only (legacy CLI / assembly).
    if(AllOfCtype(lower, std::isdigit)) {
        return ParseU16(lower, 10);
    }
    if(lower.starts_with("0x") && AllOfCtype(lower.substr(2), std::isxdigit)) {
        return ParseU16(lower.substr(2), 16);
    }

    return FindAbi(AbiName2RegIdx(), lower);
}

std::optional<uint16_t> LookupFregIdx(std::string_view target)
{
    std::string lower(target);
    AsciiLowerInPlace(lower);

    if(auto idx= LookupPrefixedIndex(lower, 'f')) return idx;
    return FindAbi(AbiName2FregIdx(), lower);
}

std::optional<uint16_t> LookupCSRIdx(std::string_view name)
{
    std::string lower(name);
    AsciiLowerInPlace(lower);
    return FindAbi(CsrName2Num(), lower);
}

} // namespace isa
