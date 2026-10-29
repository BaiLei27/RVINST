#include <algorithm>

#include "Core/InstTypeFactory.hh"
#include "Core/BType.hh"
#include "Core/IType.hh"
#include "Core/JType.hh"
#include "Core/RType.hh"
#include "Core/SType.hh"
#include "Core/UType.hh"
#include "Core/CType.hh"

namespace {

template <typename... Args>
std::unique_ptr<IBaseInstType> makeByFormat(InstFormat fmt, Args &&...args)
{
    switch(fmt) {
    case InstFormat::R:
        return std::make_unique<RType>(fmt, std::forward<Args>(args)...);
    case InstFormat::I:
        return std::make_unique<IType>(fmt, std::forward<Args>(args)...);
    case InstFormat::J:
        return std::make_unique<JType>(fmt, std::forward<Args>(args)...);
    case InstFormat::U:
        return std::make_unique<UType>(fmt, std::forward<Args>(args)...);
    case InstFormat::S:
        return std::make_unique<SType>(fmt, std::forward<Args>(args)...);
    case InstFormat::B:
        return std::make_unique<BType>(fmt, std::forward<Args>(args)...);
    case InstFormat::C:
        return std::make_unique<CType>(fmt, std::forward<Args>(args)...);
    default:
        return nullptr;
    }
}

/**
 * @brief flat_map::emplace binary-searches the insertion point and shifts the tail,
 * keeping the map sorted. The G_INST_TABLE inputs are unsorted by name, so
 * emplace (not sorted_unique) is the correct builder here.
 * @tparam N
 * @param m std::flat_map<std::string_view, std::pair<InstFormat, uint16_t>>;
 * @param table std::array<IBaseInstType::InstInfo, N>
 * @param fmt InstFormat
 */
template <size_t N>
void registerTable(InstTypeFactory::name2FormatOpcode_u &m,
                   const std::array<IBaseInstType::InstInfo, N> &table,
                   InstFormat fmt)
{
    for(const auto &entry: table) {
        if(entry.name_.empty()) continue;
        // RVC quadrant 0 uses opcode_=0; other formats use 0 as "unused" sentinel.
        if(0 == entry.opcode_ && InstFormat::C != fmt) continue;
        m.emplace(entry.name_, std::pair { fmt, entry.opcode_ });
    }
}

std::string toLower(std::string_view s)
{
    std::string lower(s);
    std::ranges::transform(lower,
                           lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}

} // namespace

std::unique_ptr<IBaseInstType> InstTypeFactory::CreateType(uint32_t inst, bool hasSetABI)
{
    if(3U != (inst & 0x3U)) return makeByFormat(InstFormat::C, inst, hasSetABI);

    const auto OPCODE= static_cast<OpcMap>(inst & 0x7FU);
    auto it          = G_OPCODE_2_FORMAT.find(OPCODE);
    if(G_OPCODE_2_FORMAT.end() == it) return nullptr;
    return makeByFormat(it->second, inst, hasSetABI);
}

const InstTypeFactory::name2FormatOpcode_u &InstTypeFactory::getName2FormatOpcode()
{
    static const name2FormatOpcode_u OPC_CACHE= [] {
        name2FormatOpcode_u m;
        registerTable(m, RType::G_INST_TABLE, InstFormat::R);
        registerTable(m, IType::G_INST_TABLE, InstFormat::I);
        registerTable(m, JType::G_INST_TABLE, InstFormat::J);
        registerTable(m, UType::G_INST_TABLE, InstFormat::U);
        registerTable(m, SType::G_INST_TABLE, InstFormat::S);
        registerTable(m, BType::G_INST_TABLE, InstFormat::B);
        registerTable(m, CType::G_INST_TABLE, InstFormat::C);
        // The string_view keys point into the immortal G_INST_TABLE literals,
        // so the ordering stays valid for the process lifetime.
        return m;
    }();
    return OPC_CACHE;
}

std::unique_ptr<IBaseInstType> InstTypeFactory::CreateType(std::vector<std::string> instAssembly,
                                                           bool hasSetABI)
{
    if(instAssembly.empty()) return nullptr;
    auto &instName= instAssembly[0];

    instName= toLower(instName);

    auto match= matchInstName(instName);
    if(!match) return nullptr;

    const auto FMT= match->first;
    return makeByFormat(FMT, std::move(instAssembly), hasSetABI);
}

std::optional<std::pair<InstFormat, uint16_t>>
    InstTypeFactory::matchInstName(std::string_view instName)
{
    const auto &cache= getName2FormatOpcode();
    auto it          = cache.find(instName);
    return it != cache.end() ? std::make_optional(it->second) : std::nullopt;
}

bool InstTypeFactory::IsKnownMnemonic(std::string_view name) noexcept
{
    return matchInstName(toLower(name)).has_value();
}
