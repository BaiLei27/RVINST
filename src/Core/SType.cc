#include <print>
#include <string>

#include "Core/SType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

int32_t decodeSImm12(const InstLayout &l)
{
    const int32_t S_IMM= (l.S.imm5tB << 5) | l.S.imm0t4;
    return (S_IMM << 20) >> 20;
}

void encodeSImm12(InstLayout &l, int32_t imm)
{
    const uint32_t S_IMM= imm & 0xFFFU;

    l.S.imm0t4= S_IMM;
    l.S.imm5tB= S_IMM >> 5;
}

bool parseStoreAddr(std::string_view token, int32_t &immOut, std::string &regStrOut)
{
    const auto L_PAREN= token.find('(');
    const auto R_PAREN= token.find(')');
    if(std::string_view::npos == L_PAREN || std::string_view::npos == R_PAREN
       || R_PAREN <= L_PAREN + 1U) {
        return false;
    }
    std::string immPart(token.substr(0, L_PAREN));
    regStrOut.assign(token.substr(L_PAREN + 1, R_PAREN - L_PAREN - 1));
    return util::ParseInt(immPart, immOut);
}

} // namespace

SType::SType(uint32_t inst, InstFormat format, bool hasSetABI)
    : IBaseInstType(inst, format, hasSetABI)
{
    init();
}

SType::SType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI)
    : IBaseInstType(std::move(instAssembly), format, hasSetABI)
{
    init();
}

void SType::Parse()
{
    setBitsField({ Layout_.S.opc,
                   Layout_.S.imm0t4,
                   Layout_.S.fct3,
                   Layout_.S.rs1,
                   Layout_.S.rs2,
                   Layout_.S.imm5tB });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrs2: {}\nimm: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.S.fct3,
                 +Layout_.S.rs1,
                 +Layout_.S.rs2,
                 decodeSImm12(Layout_));
#endif
}

void SType::mnemonicHelper()
{
    auto rs2          = isa::LOOKUP_REG_NAME(Layout_.S.rs2, HasSetABI_);
    auto rs1          = isa::LOOKUP_REG_NAME(Layout_.S.rs1, HasSetABI_);
    std::string immStr= std::to_string(decodeSImm12(Layout_));
    appendOperands({ " ", rs2, ",", immStr, "(", rs1, ")" });
}

const InstLayout &SType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.S.opc= info.opcode_;
    Layout_.S.fct3        = info.funct_ & 7U;

    if(InstAssembly_.size() >= 4U) {
        if(auto rs2Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.S.rs2= *rs2Opt;
        }
        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(2), imm)) {
            encodeSImm12(Layout_, imm);
        }
        if(auto rs1Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(3))) {
            Layout_.S.rs1= *rs1Opt;
        }
    } else if(InstAssembly_.size() >= 3U) {
        if(auto rs2Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.S.rs2= *rs2Opt;
        }
        int32_t imm {};
        std::string baseReg;
        if(parseStoreAddr(InstAssembly_.at(2), imm, baseReg)) {
            encodeSImm12(Layout_, imm);
            if(auto rs1Opt= isa::LOOKUP_REG_IDX(baseReg)) {
                Layout_.S.rs1= *rs1Opt;
            }
        }
    }

    mnemonicHelper();
    return Layout_;
}

IBaseInstType::KeyT SType::calculateFunctKey()
{
    FunctKey_= static_cast<KeyT>((Layout_.S.opc << 8) | (Layout_.S.fct3 & 7U));
    return FunctKey_;
}

const BiLookupTable<IBaseInstType::KeyT> *SType::buildTable()
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
