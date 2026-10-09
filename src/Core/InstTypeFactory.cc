#include <array>
#include <print>

#include "Core/InstTypeFactory.hh"
#include "Core/BType.hh"
#include "Core/IType.hh"
#include "Core/JType.hh"
#include "Core/RType.hh"
#include "Core/SType.hh"
#include "Core/UType.hh"

namespace {

template <typename... Args>
std::unique_ptr<IBaseInstType> makeByFormat(InstFormat fmt, Args &&...args)
{
    switch(fmt) {
    case InstFormat::R:
        return std::make_unique<RType>(std::forward<Args>(args)...);
    case InstFormat::I:
        return std::make_unique<IType>(std::forward<Args>(args)...);
    case InstFormat::J:
        return std::make_unique<JType>(std::forward<Args>(args)...);
    case InstFormat::U:
        return std::make_unique<UType>(std::forward<Args>(args)...);
    case InstFormat::S:
        return std::make_unique<SType>(std::forward<Args>(args)...);
    case InstFormat::B:
        return std::make_unique<BType>(std::forward<Args>(args)...);
    default:
        std::println(stderr, "Unsupported instruction format");
        return nullptr;
    }
}

using Name2FmtOpcode_u= std::unordered_map<std::string_view, std::pair<InstFormat, uint16_t>>;

template <size_t N>
void registerTable(Name2FmtOpcode_u &m, const std::array<IBaseInstType::InstInfo, N> &table, InstFormat fmt)
{
    for(const auto &entry: table) {
        if(0 == entry.opcode_ || entry.name_.empty()) continue;
        m.emplace(entry.name_, std::pair { fmt, entry.opcode_ });
    }
}

} // namespace

std::unique_ptr<IBaseInstType> InstTypeFactory::CreateType(uint32_t inst, bool hasSetABI)
{
    const uint16_t OPCODE= inst & 0x7F;
    auto it              = G_Opcode2Format.find(OPCODE);
    if(G_Opcode2Format.end() == it) return nullptr;
    return makeByFormat(it->second, inst, it->second, hasSetABI);
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
        return m;
    }();
    return OPC_CACHE;
}

std::unique_ptr<IBaseInstType> InstTypeFactory::CreateType(std::vector<std::string> instAssembly,
                                                           bool hasSetABI)
{
    if(instAssembly.empty()) return nullptr;

    auto match= matchInstName(instAssembly[0]);
    if(!match) {
        std::println(stderr, "Unsupported instruction name: {}", instAssembly[0]);
        return nullptr;
    }

    const auto [FMT, _1]= *match;
    return makeByFormat(FMT, std::move(instAssembly), FMT, hasSetABI);
}

std::optional<std::pair<InstFormat, uint16_t>> InstTypeFactory::matchInstName(std::string_view instName)
{
    const auto &cache= getName2FormatOpcode();
    auto it          = cache.find(instName);
    return it != cache.end() ? std::make_optional(it->second) : std::nullopt;
}
