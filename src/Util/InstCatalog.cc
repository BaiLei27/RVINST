#include <cctype>

#include "Core/BType.hh"
#include "Core/CType.hh"
#include "Core/IType.hh"
#include "Core/JType.hh"
#include "Core/RType.hh"
#include "Core/R4Type.hh"
#include "Core/SType.hh"
#include "Core/UType.hh"
#include "Util/InstCatalog.hh"

namespace util {
namespace {

    // Lowercase alnum only: "R4-Type" / "r-4" → "r4type" / "r4".
    [[nodiscard]] std::string NormalizeFormatToken(std::string_view s)
    {
        std::string out;
        out.reserve(s.size());
        for(const auto &c: s) {
            if('-' == c || '_' == c || ' ' == c) continue;
            out.push_back(static_cast<char>(std::tolower(c)));
        }
        return out;
    }

} // namespace

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
    if(!filter || InstFormat::R4 == *filter) addTable(R4Type::G_INST_TABLE, InstFormat::R4);
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
    if(s.empty()) return std::nullopt;

    const std::string N= NormalizeFormatToken(s);

    // R4 before R: "r4" / "r4type" must not collapse to InstFormat::R
    if("r4" == N || "r4type" == N || "r4t" == N) return InstFormat::R4;
    if("r" == N || "rtype" == N || "rt" == N) return InstFormat::R;
    if("i" == N || "itype" == N || "it" == N) return InstFormat::I;
    if("s" == N || "stype" == N || "st" == N) return InstFormat::S;
    if("b" == N || "btype" == N || "bt" == N) return InstFormat::B;
    if("u" == N || "utype" == N || "ut" == N) return InstFormat::U;
    if("j" == N || "jtype" == N || "jt" == N) return InstFormat::J;
    if("c" == N || "ctype" == N || "ct" == N) return InstFormat::C;

    return std::nullopt;
}

} // namespace util
