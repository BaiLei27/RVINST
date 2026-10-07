#include <print>

#include "Core/JType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

int32_t decodeJImm(const InstLayout &l)
{
    int32_t sImm= (l.J.imm14 << 20)
                | (l.J.imm1tA << 1)
                | (l.J.immB << 11)
                | (l.J.immCt13 << 12);

    if(uint32_t signedBit= 1U << 20; (sImm & signedBit) != 0U) {
        sImm|= ~0x1FFFFF;
    }

    return sImm;
}

void encodeJImm(InstLayout &l, int32_t imm)
{
    int32_t sImm= imm & 0x1FFFFF;

    l.J.imm14  = sImm >> 20;
    l.J.imm1tA = sImm >> 1;
    l.J.immB   = sImm >> 11;
    l.J.immCt13= sImm >> 12;
}

} // namespace

JType::JType(uint32_t inst, InstFormat format, bool hasSetABI)
    : IBaseInstType(inst, format, hasSetABI)
{
    init();
}

JType::JType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI)
    : IBaseInstType(std::move(instAssembly), format, hasSetABI)
{
    init();
}

void JType::Parse()
{
    InstBitsField_.push_back(Layout_.J.opc);
    InstBitsField_.push_back(Layout_.J.rd);
    InstBitsField_.push_back(Layout_.J.immCt13);
    InstBitsField_.push_back(Layout_.J.immB);
    InstBitsField_.push_back(Layout_.J.imm1tA);
    InstBitsField_.push_back(Layout_.J.imm14);

    int32_t imm= decodeJImm(Layout_);
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nrd: {}\nimm: {}",
                 Opcode_,
                 Layout_.entity_,
                 +Layout_.J.rd,
                 imm);
#endif
}

void JType::mnemonicHelper()
{
    auto rd           = isa::LOOKUP_REG_NAME(Layout_.J.rd, HasSetABI_);
    int32_t imm       = decodeJImm(Layout_);
    std::string immStr= std::to_string(imm);

    appendOperands({ " ", rd, ",", immStr });
}

const std::vector<std::string> &JType::Disassembly()
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

const InstLayout &JType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.J.opc= Opcode_= info.opcode_;

    if(!InstAssembly_.empty() && InstAssembly_.size() >= 3) {
        if(auto rdOpt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.J.rd= *rdOpt;
        }
        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(2), imm)) {
            encodeJImm(Layout_, imm);
        }
    }

    mnemonicHelper();

    return Layout_;
}

IBaseInstType::KeyT JType::calculateFunctKey()
{
    FunctKey_= 0;
    return FunctKey_;
}

IBaseInstType::pBiTable_u JType::buildTable()
{
    static auto s_instTable= [](const std::string &baseURL) -> pBiTable_u {
        BiLookupTable<KeyT>::intMapName_u code2info;
        BiLookupTable<KeyT>::strMapIndex_u name2info;

        for(const auto &entry: G_INST_TABLE) {
            if(0x00 == entry.opcode_ || entry.XLEN_.empty() || entry.name_.empty()) {
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
