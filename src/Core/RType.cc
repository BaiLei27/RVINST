#include "Core/RType.hh"
#include "ISA/Regs.hpp"

namespace {

[[nodiscard]] bool nameStartsWith(std::string_view name, std::string_view prefix)
{
    return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
}

} // namespace

RType::RType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
}

RType::RType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(format, std::move(instAssembly), hasSetABI)
{
    init();
}

bool RType::isAmoName(std::string_view name) noexcept
{
    return nameStartsWith(name, "amo") || nameStartsWith(name, "lr.") || nameStartsWith(name, "sc.");
}

void RType::Parse()
{
    // AMO overlays R; fct7 holds funct5|aq|rl for field display.
    setBitsField({ Layout_.R.opc,
                   Layout_.R.rd,
                   Layout_.R.fct3,
                   Layout_.R.rs1,
                   Layout_.R.rs2,
                   Layout_.R.fct7 });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nfunct7: {}\nrs1: {}\nrs2: {}\nrd: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.R.fct3,
                 +Layout_.R.fct7,
                 +Layout_.R.rs1,
                 +Layout_.R.rs2,
                 +Layout_.R.rd);
#endif
}

void RType::mnemonicHelper()
{
    // Encode path has no NameAndXlenCache_; prefer the mnemonic already in InstAssembly_.
    const auto NAME= !InstAssembly_.empty() ? std::string_view { InstAssembly_[0] }
                                            : LookupNameAndInfo().name_;

    if(OpcMap::AMO == Layout_.R.opc || isAmoName(NAME)) {
        auto rd = isa::LOOKUP_REG_NAME(Layout_.A.rd, HasSetABI_);
        auto rs1= isa::LOOKUP_REG_NAME(Layout_.A.rs1, HasSetABI_);
        if(nameStartsWith(NAME, "lr.")) {
            appendOperands({ " ", rd, ",", "(", rs1, ")" });
        } else {
            auto rs2= isa::LOOKUP_REG_NAME(Layout_.A.rs2, HasSetABI_);
            appendOperands({ " ", rd, ",", rs2, ",", "(", rs1, ")" });
        }
        if(Layout_.A.aq || Layout_.A.rl) {
            std::string tagged { NAME };
            if(!tagged.ends_with(".aq") && !tagged.ends_with(".rl")) {
                if(Layout_.A.aq) tagged+= ".aq";
                if(Layout_.A.rl) tagged+= ".rl";
            }
            if(!InstAssembly_.empty()) InstAssembly_[0]= std::move(tagged);
        }
        return;
    }

    auto rd = isa::LOOKUP_REG_NAME(Layout_.R.rd, HasSetABI_);
    auto rs1= isa::LOOKUP_REG_NAME(Layout_.R.rs1, HasSetABI_);
    auto rs2= isa::LOOKUP_REG_NAME(Layout_.R.rs2, HasSetABI_);
    appendOperands({ " ", rd, ",", rs1, ",", rs2 });
}

util::Expected_u<const InstLayout *> RType::Assembly()
{
    const auto &info  = LookupIdxAndInfo();
    Layout_.R.opc     = info.opcode_;
    const auto PAYLOAD= KEY_PAYLOAD(info.keyID_);
    const auto NAME   = InstAssembly_.empty() ? std::string_view {} : std::string_view { InstAssembly_[0] };

    if(OpcMap::AMO == info.opcode_) {
        Layout_.A.fct5= PAYLOAD >> 3;
        Layout_.A.fct3= PAYLOAD;
        Layout_.A.aq  = 0;
        Layout_.A.rl  = 0;

        if(InstAssembly_.size() >= 3U) {
            if(auto rd= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) Layout_.A.rd= *rd;
            if(nameStartsWith(NAME, "lr.")) {
                std::string_view base= InstAssembly_.at(2);
                if(base.starts_with('(') && base.ends_with(')')) {
                    base= base.substr(1, base.size() - 2);
                }
                if(auto rs1= isa::LOOKUP_REG_IDX(base)) Layout_.A.rs1= *rs1;
                Layout_.A.rs2= 0;
            } else if(InstAssembly_.size() >= 4U) {
                if(auto rs2= isa::LOOKUP_REG_IDX(InstAssembly_.at(2))) Layout_.A.rs2= *rs2;
                std::string_view base= InstAssembly_.at(3);
                if(base.starts_with('(') && base.ends_with(')')) {
                    base= base.substr(1, base.size() - 2);
                }
                if(auto rs1= isa::LOOKUP_REG_IDX(base)) Layout_.A.rs1= *rs1;
            }
        }
        mnemonicHelper();
        return &Layout_;
    }

    Layout_.R.fct7= PAYLOAD >> 3;
    Layout_.R.fct3= PAYLOAD;

    if(InstAssembly_.size() >= 4U) {
        auto rdOpt = isa::LOOKUP_REG_IDX(InstAssembly_.at(1));
        auto rs1Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(2));
        auto rs2Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(3));
        if(rdOpt && rs1Opt && rs2Opt) {
            Layout_.R.rd = *rdOpt;
            Layout_.R.rs1= *rs1Opt;
            Layout_.R.rs2= *rs2Opt;
        }
    }

    mnemonicHelper();
    return &Layout_;
}

auto RType::calculateKeyID() -> KeyT
{
    if(OpcMap::AMO == Layout_.R.opc) {
        return Key_= AMO_K(Layout_.A.fct5, Layout_.A.fct3, Layout_.A.opc);
    }
    return Key_= OP_K(Layout_.R.fct7, Layout_.R.fct3, Layout_.R.opc);
}

auto RType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
