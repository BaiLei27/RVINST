#include <print>
#include <string>

#include "Core/BType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

int32_t decodeBImm13(const InstLayout &l)
{
    int32_t sImm= (l.B.immC << 12)
             | (l.B.immB << 11)
             | (l.B.imm5tA << 5)
             | (l.B.imm1t4 << 1);

    return (sImm << 19) >> 19;
}

void encodeBImm13(InstLayout &l, int32_t imm)
{
    int32_t sImm= imm & 0x1FFF;

    l.B.imm1t4= sImm >> 1;
    l.B.immB  = sImm >> 11;
    l.B.imm5tA= sImm >> 5;
    l.B.immC  = sImm >> 12;
}

} // namespace

BType::BType(uint32_t inst, InstFormat format, bool hasSetABI)
    : IBaseInstType(inst, format, hasSetABI)
{
    init();
}

BType::BType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI)
    : IBaseInstType(std::move(instAssembly), format, hasSetABI)
{
    init();
}

void BType::Parse()
{
    InstBitsField_.push_back(Layout_.B.opc);
    InstBitsField_.push_back(Layout_.B.immB);
    InstBitsField_.push_back(Layout_.B.imm1t4);
    InstBitsField_.push_back(Layout_.B.fct3);
    InstBitsField_.push_back(Layout_.B.rs1);
    InstBitsField_.push_back(Layout_.B.rs2);
    InstBitsField_.push_back(Layout_.B.imm5tA);
    InstBitsField_.push_back(Layout_.B.immC);

#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrs2: {}\nimm: {}",
                 Opcode_,
                 Layout_.entity_,
                 +Layout_.B.fct3,
                 +Layout_.B.rs1,
                 +Layout_.B.rs2,
                 decodeBImm13(Layout_));
#endif
}

void BType::mnemonicHelper()
{
    auto rs1   = isa::LOOKUP_REG_NAME(Layout_.B.rs1, HasSetABI_);
    auto rs2   = isa::LOOKUP_REG_NAME(Layout_.B.rs2, HasSetABI_);
    auto immStr= std::to_string(decodeBImm13(Layout_));
    appendOperands({ " ", rs1, ",", rs2, ",", immStr });
}

const std::vector<std::string> &BType::Disassembly()
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

const InstLayout &BType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.B.opc= Opcode_= info.opcode_;
    Layout_.B.fct3        = info.funct_ & 7U;

    if(InstAssembly_.size() >= 4U) {
        if(auto r1= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.B.rs1= *r1;
        }
        if(auto r2= isa::LOOKUP_REG_IDX(InstAssembly_.at(2))) {
            Layout_.B.rs2= *r2;
        }
        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(3), imm)) {
            encodeBImm13(Layout_, imm);
        }
    }

    mnemonicHelper();

    return Layout_;
}

IBaseInstType::KeyT BType::calculateFunctKey()
{
    FunctKey_= (Opcode_ << 8) | (Layout_.B.fct3 & 7U);
    return FunctKey_;
}

IBaseInstType::pBiTable_u BType::buildTable()
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
