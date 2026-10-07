#include <cctype>

#include "Core/BType.hh"
#include "Core/IType.hh"
#include "Core/JType.hh"
#include "Core/RType.hh"
#include "Core/SType.hh"
#include "Core/UType.hh"
#include "Cli/InstrEntry.hh"

namespace cli {

std::vector<InstrEntry> getAllInstructions(std::optional<InstFormat> filter)
{
    std::vector<InstrEntry> out;

    auto addTable= [&](const auto &table, InstFormat fmt) {
        for(const auto &e: table) {
            if(0 == e.opcode_ || e.name_.empty()) continue;
            out.emplace_back(e.name_, e.XLEN_, e.funct_, e.opcode_, fmt);
        }
    };

    if(!filter || InstFormat::R == *filter) addTable(RType::G_INST_TABLE, InstFormat::R);
    if(!filter || InstFormat::I == *filter) addTable(IType::G_INST_TABLE, InstFormat::I);
    if(!filter || InstFormat::S == *filter) addTable(SType::G_INST_TABLE, InstFormat::S);
    if(!filter || InstFormat::B == *filter) addTable(BType::G_INST_TABLE, InstFormat::B);
    if(!filter || InstFormat::U == *filter) addTable(UType::G_INST_TABLE, InstFormat::U);
    if(!filter || InstFormat::J == *filter) addTable(JType::G_INST_TABLE, InstFormat::J);

    return out;
}

std::optional<InstFormat> stringToFormat(std::string_view s)
{
    if(s.size() == 1 || (s.size() >= 2 && (s[1] == '-' || std::tolower(static_cast<char>(s[1])) == 't'))) {
        switch(std::toupper(static_cast<char>(s.front()))) {
        case 'R': return InstFormat::R;
        case 'I': return InstFormat::I;
        case 'S': return InstFormat::S;
        case 'B': return InstFormat::B;
        case 'U': return InstFormat::U;
        case 'J': return InstFormat::J;
        default:  break;
        }
    }
    if(s == "r" || s == "R") return InstFormat::R;
    if(s == "i" || s == "I") return InstFormat::I;
    if(s == "s" || s == "S") return InstFormat::S;
    if(s == "b" || s == "B") return InstFormat::B;
    if(s == "u" || s == "U") return InstFormat::U;
    if(s == "j" || s == "J") return InstFormat::J;

    return std::nullopt;
}

} // namespace cli
