#include <print>
#include <string>

#include "Core/SType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

int32_t decodeSImm12(const InstLayout &l)
{
    int32_t u= (l.S.imm5tB << 5) | l.S.imm0t4;

    return (u << 20) >> 20;
}

void encodeSImm12(InstLayout &l, int32_t imm)
{
    uint32_t u= imm & 0xFFFU;
    l.S.imm0t4= u;
    l.S.imm5tB= u >> 5;
}

bool parseStoreAddr(std::string_view token, int32_t &immOut, std::string &regStrOut)
{
    auto lParen= token.find('(');
    auto rParen= token.find(')');
    if(lParen == std::string_view::npos || rParen == std::string_view::npos || rParen <= lParen + 1U) {
        return false;
    }
    std::string immPart(token.substr(0, lParen));
    regStrOut.assign(token.substr(lParen + 1, rParen - lParen - 1));
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
    InstBitsField_.push_back(Layout_.S.opc);
    InstBitsField_.push_back(Layout_.S.imm0t4);
    InstBitsField_.push_back(Layout_.S.fct3);
    InstBitsField_.push_back(Layout_.S.rs1);
    InstBitsField_.push_back(Layout_.S.rs2);
    InstBitsField_.push_back(Layout_.S.imm5tB);

#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrs2: {}\nimm: {}",
                 Opcode_,
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

const std::vector<std::string> &SType::Disassembly()
{
    if(!InstTable_) {
        InstTable_= buildTable();
    }

    if(InstAssembly_.empty()) {
        const auto &info= LookupNameAndInfo();
        InstAssembly_.emplace_back(info.name_);
        mnemonicHelper();
    }

    return InstAssembly_;
}

const InstLayout &SType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.S.opc= Opcode_= info.opcode_;
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
    FunctKey_= (Opcode_ << 8) | (Layout_.S.fct3 & 7U);
    return FunctKey_;
}

IBaseInstType::pBiTable_u SType::buildTable()
{
    static auto s_instTable= [](const std::string &baseURL) -> pBiTable_u {
        BiLookupTable<KeyT>::intMapName_u code2info;
        BiLookupTable<KeyT>::strMapIndex_u name2info;

        for(const auto &entry: G_INST_TABLE) {
            if(0U == entry.opcode_ || entry.XLEN_.empty() || entry.name_.empty()) {
                continue;
            }
            auto manualURL= baseURL + std::string(entry.name_);

            code2info.emplace(entry.funct_,
                              BiLookupTable<KeyT>::NameInfo { .manual_= manualURL,
                                                              .XLEN_  = entry.XLEN_,
                                                              .name_  = entry.name_ });
            name2info.emplace(entry.name_,
                              BiLookupTable<KeyT>::IndexInfo { .manual_= manualURL,
                                                               .XLEN_  = entry.XLEN_,
                                                               .funct_ = entry.funct_,
                                                               .opcode_= entry.opcode_ });
        }

        return std::make_shared<const BiLookupTable<KeyT>>(std::move(code2info), std::move(name2info));
    }(BaseURL_);

    return s_instTable;
}
