#include <utility>

#include "Core/IBaseInstType.hh"

IBaseInstType::IBaseInstType(uint32_t inst, InstFormat format, bool hasSetABI)
    : Format_(format),
      Layout_(inst),
      HasSetABI_(hasSetABI) { }

IBaseInstType::IBaseInstType(std::vector<std::string> instAssembly,
                             InstFormat format,
                             bool hasSetABI)
    : InstAssembly_(std::move(instAssembly)),
      Format_(format),
      HasSetABI_(hasSetABI) { }

const InstLayout &IBaseInstType::GetInstLayout() const noexcept { return Layout_; }

const std::vector<std::string> &IBaseInstType::GetInstAssembly() const noexcept { return InstAssembly_; }

const std::vector<uint32_t> &IBaseInstType::GetInstBitsField() const noexcept { return InstBitsField_; }

uint16_t IBaseInstType::GetInstOpcode() const noexcept { return Layout_.entity_ & 0x7FU; }

uint16_t IBaseInstType::GetInstFunctKey() const noexcept { return FunctKey_; }

InstFormat IBaseInstType::GetInstFormat() const noexcept { return Format_; }

// std::string_view IBaseInstType::GetInstDisassembly() const { return InstDisassembly_; }
void IBaseInstType::SetInstAssembly(std::vector<std::string> instAssembly) { InstAssembly_= std::move(instAssembly); }

void IBaseInstType::SetFormat(InstFormat format) noexcept { Format_= format; }

void IBaseInstType::init()
{
    InstTable_= buildTable();
    if(!InstTable_) return;

    if(InstAssembly_.empty()) {
        calculateFunctKey();
        NameAndXlenCache_= InstTable_->Find(FunctKey_);
    } else {
        FunctOpcAndXlenCache_= InstTable_->Find(InstAssembly_.at(0));
    }
}

const BiLookupTable<IBaseInstType::KeyT>::NameInfo &IBaseInstType::LookupNameAndInfo() const
{
    if(NameAndXlenCache_) return *NameAndXlenCache_;

    static const BiLookupTable<KeyT>::NameInfo K_UNDEF {
        .manual_= "Not available",
        .XLEN_  = "UNDEF",
        .name_  = "unimp",
    };
    return K_UNDEF;
}

BiLookupTable<IBaseInstType::KeyT>::IndexInfo IBaseInstType::LookupIdxAndInfo() const
{
    if(FunctOpcAndXlenCache_) return *FunctOpcAndXlenCache_;
    return { .manual_= "Not available",
             .XLEN_  = "UNDEF",
             .funct_ = FunctKey_,
             .opcode_= GetInstOpcode() };
}

const std::vector<std::string> &IBaseInstType::Disassembly()
{
    if(!InstTable_) InstTable_= buildTable();

    if(InstAssembly_.empty()) {
        InstAssembly_.emplace_back(LookupNameAndInfo().name_);
        mnemonicHelper();
    }
    return InstAssembly_;
}

void IBaseInstType::appendOperands(std::initializer_list<std::string_view> regMnemonic)
{

    // for(const auto *pIt= regMnemonic.begin(); pIt != regMnemonic.end(); ++pIt) { // NOLINT
    //     InstAssembly_.emplace_back(*pIt);
    //     if(std::next(pIt) != regMnemonic.end()) {
    //         InstAssembly_.emplace_back(", ");
    //     }
    // }

    size_t writeIdx= 1;
    for(const auto &r: regMnemonic) {
        if(writeIdx >= InstAssembly_.size()) {
            InstAssembly_.emplace_back(r);
        } else {
            InstAssembly_[writeIdx]= r;
        }
        ++writeIdx;
    }
    if(InstAssembly_.size() > writeIdx) {
        InstAssembly_.resize(writeIdx);
    }
}

void IBaseInstType::setBitsField(std::initializer_list<uint32_t> bits)
{
    InstBitsField_.assign(bits);
}

void IBaseInstType::setBitsField(std::span<const uint32_t> bits)
{
    InstBitsField_.assign(bits.begin(), bits.end());
}

const BiLookupTable<IBaseInstType::KeyT> *
    IBaseInstType::makeLookupTable(std::span<const InstInfo> entries, std::string_view baseURL)
{
    BiLookupTable<KeyT>::intMapName_u code2info;
    BiLookupTable<KeyT>::strMapIndex_u name2info;
    code2info.reserve(entries.size());
    name2info.reserve(entries.size());

    for(const auto &entry: entries) {
        if(0U == entry.opcode_ || entry.XLEN_.empty() || entry.name_.empty()) continue;

        std::string manualURL;
        manualURL.reserve(baseURL.size() + entry.name_.size());
        manualURL.append(baseURL);
        manualURL.append(entry.name_);

        name2info.emplace(entry.name_,
                          BiLookupTable<KeyT>::IndexInfo { .manual_= manualURL,
                                                           .XLEN_  = entry.XLEN_,
                                                           .funct_ = entry.funct_,
                                                           .opcode_= entry.opcode_ });
        code2info.emplace(entry.funct_,
                          BiLookupTable<KeyT>::NameInfo { .manual_= std::move(manualURL),
                                                          .XLEN_  = entry.XLEN_,
                                                          .name_  = entry.name_ });
    }

    // Immortal per-format table; ownership kept by the call-site static holder.
    return new BiLookupTable<KeyT>(std::move(code2info), std::move(name2info));
}
