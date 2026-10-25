#include <cctype>

#include "Core/BType.hh"
#include "Core/CType.hh"
#include "Core/IType.hh"
#include "Core/JType.hh"
#include "Core/RType.hh"
#include "Core/SType.hh"
#include "Core/UType.hh"
#include "Util/InstCatalog.hh"

namespace util {

std::vector<InstCatalog> GetAllInstructions(std::optional<InstFormat> filter)
{
    std::vector<InstCatalog> out;

    auto addTable= [&](const auto &table, InstFormat fmt) {
        for(const auto &e: table) {
            if(e.name_.empty()) continue;
            if(0 == e.opcode_ && InstFormat::C != fmt) continue;
            out.emplace_back(e.name_, e.XLEN_, e.keyID_, e.opcode_, fmt);
        }
    };

    if(!filter || InstFormat::R == *filter) addTable(RType::G_INST_TABLE, InstFormat::R);
    if(!filter || InstFormat::I == *filter) addTable(IType::G_INST_TABLE, InstFormat::I);
    if(!filter || InstFormat::S == *filter) addTable(SType::G_INST_TABLE, InstFormat::S);
    if(!filter || InstFormat::B == *filter) addTable(BType::G_INST_TABLE, InstFormat::B);
    if(!filter || InstFormat::U == *filter) addTable(UType::G_INST_TABLE, InstFormat::U);
    if(!filter || InstFormat::J == *filter) addTable(JType::G_INST_TABLE, InstFormat::J);
    if(!filter || InstFormat::C == *filter) addTable(CType::G_INST_TABLE, InstFormat::C);

    return out;
}

std::optional<InstFormat> StringToFormat(std::string_view s)
{
    if(s.size() == 1 || (s.size() >= 2 && (s[1] == '-' || std::tolower(static_cast<char>(s[1])) == 't'))) {
        switch(std::toupper(static_cast<char>(s.front()))) {
        case 'R': return InstFormat::R;
        case 'I': return InstFormat::I;
        case 'S': return InstFormat::S;
        case 'B': return InstFormat::B;
        case 'U': return InstFormat::U;
        case 'J': return InstFormat::J;
        case 'C': return InstFormat::C;
        default:  break;
        }
    }
    if(s == "r" || s == "R") return InstFormat::R;
    if(s == "i" || s == "I") return InstFormat::I;
    if(s == "s" || s == "S") return InstFormat::S;
    if(s == "b" || s == "B") return InstFormat::B;
    if(s == "u" || s == "U") return InstFormat::U;
    if(s == "j" || s == "J") return InstFormat::J;
    if(s == "c" || s == "C") return InstFormat::C;

    return std::nullopt;
}

} // namespace util
