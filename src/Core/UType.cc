#include <print>
#include <string>

#include "Core/UType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

uint32_t parseAsmImm20(std::string_view tok)
{
    uint32_t v {};
    if(!util::ParseInt(tok, v)) {
        return 0;
    }
    return v <= 0xFFFFF ? v : v >> 12;
}

} // namespace

UType::UType(uint32_t inst, InstFormat format, bool hasSetABI)
    : IBaseInstType(inst, format, hasSetABI)
{
    init();
}

UType::UType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI)
    : IBaseInstType(std::move(instAssembly), format, hasSetABI)
{
    init();
}

void UType::Parse()
{
    InstBitsField_.push_back(Layout_.U.opc);
    InstBitsField_.push_back(Layout_.U.rd);
    InstBitsField_.push_back(Layout_.U.immCt1F);

#ifdef DEBUG_
    std::println(std::cout,
                 "opcode: 0x{:x}\nHexadecimal: 0x{:x}\nrd: {}\nimm[31:12]: {}",
                 Opcode_,
                 Layout_.entity_,
                 +Layout_.U.rd,
                 Layout_.U.immCt1F);
#endif
}

void UType::mnemonicHelper()
{
    auto rd           = isa::LOOKUP_REG_NAME(Layout_.U.rd, HasSetABI_);
    std::string immStr= std::to_string(Layout_.U.immCt1F);
    appendOperands({ " ", rd, ",", immStr });
}

const std::vector<std::string> &UType::Disassembly()
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

const InstLayout &UType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.U.opc= Opcode_= info.opcode_;

    if(InstAssembly_.size() >= 3) {
        if(auto rdOpt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.U.rd= *rdOpt;
        }
        Layout_.U.immCt1F= parseAsmImm20(InstAssembly_.at(2));
    }

    mnemonicHelper();

    return Layout_;
}

IBaseInstType::KeyT UType::calculateFunctKey()
{
    FunctKey_= Opcode_;
    return FunctKey_;
}

IBaseInstType::pBiTable_u UType::buildTable()
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
