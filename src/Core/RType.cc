#include "Core/RType.hh"
#include "ISA/Regs.hpp"

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

void RType::Parse()
{
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
                 +Layout_.R.rd); // s.x <==> static_cast<int>(s.x)
#endif
}

void RType::mnemonicHelper()
{
    auto rd = isa::LOOKUP_REG_NAME(Layout_.R.rd, HasSetABI_); // actually reg mnemonic only 5b (max: 31),never overflow
    auto rs1= isa::LOOKUP_REG_NAME(Layout_.R.rs1, HasSetABI_);
    auto rs2= isa::LOOKUP_REG_NAME(Layout_.R.rs2, HasSetABI_);
    appendOperands({ " ", rd, ",", rs1, ",", rs2 });
}

util::Expected_u<const InstLayout *> RType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.R.opc     = info.opcode_;
    const auto PAYLOAD= KEY_PAYLOAD(info.keyID_);
    Layout_.R.fct7    = PAYLOAD >> 3;
    Layout_.R.fct3    = PAYLOAD;

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
    return Key_= OP_K(Layout_.R.fct7, Layout_.R.fct3, Layout_.R.opc);
}

auto RType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
