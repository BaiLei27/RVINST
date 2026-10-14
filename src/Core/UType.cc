#include "Core/UType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hh"

namespace {

uint32_t parseAsmImm20(std::string_view tok)
{
    uint32_t v {};
    if(!util::ParseInt(tok, v)) return 0;
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
    setBitsField({ Layout_.U.opc, Layout_.U.rd, Layout_.U.immCt1F });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nrd: {}\nimm[31:12]: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.U.rd,
                 +Layout_.U.immCt1F); //+s.x <==> static_cast<int>(s.x)
#endif
}

void UType::mnemonicHelper()
{
    auto rd           = isa::LOOKUP_REG_NAME(Layout_.U.rd, HasSetABI_);
    std::string immStr= std::to_string(Layout_.U.immCt1F);
    appendOperands({ " ", rd, ",", immStr });
}

util::Expected_u<const InstLayout *> UType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.U.opc= info.opcode_;

    if(InstAssembly_.size() >= 3) {
        if(auto rdOpt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.U.rd= *rdOpt;
        }
        Layout_.U.immCt1F= parseAsmImm20(InstAssembly_.at(2));
    }

    mnemonicHelper();
    return &Layout_;
}

auto UType::calculateKeyID() -> KeyT
{
    return Key_= OP_K(Layout_.U.opc);
}

auto UType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
