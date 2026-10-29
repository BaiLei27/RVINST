#include <string>

#include "Core/R4Type.hh"
#include "ISA/Regs.hh"

namespace {

constexpr uint8_t G_RM_DYN= 7;

} // namespace

R4Type::R4Type(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(InstFormat::R4, inst, hasSetABI)
{
    (void)format; // factory always passes InstFormat::R4; pin it so the format is never mislabeled
    init();
}

R4Type::R4Type(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(InstFormat::R4, std::move(instAssembly), hasSetABI)
{
    (void)format; // factory always passes InstFormat::R4; pin it so the format is never mislabeled
    init();
}

void R4Type::Parse()
{
    setBitsField({ Layout_.R4.opc,
                   Layout_.R4.rd,
                   Layout_.R4.rm,
                   Layout_.R4.rs1,
                   Layout_.R4.rs2,
                   Layout_.R4.fmt,
                   Layout_.R4.rs3 });
}

void R4Type::mnemonicHelper()
{
    auto rd = isa::LookupFregName(Layout_.R4.rd, HasSetABI_);
    auto rs1= isa::LookupFregName(Layout_.R4.rs1, HasSetABI_);
    auto rs2= isa::LookupFregName(Layout_.R4.rs2, HasSetABI_);
    auto rs3= isa::LookupFregName(Layout_.R4.rs3, HasSetABI_);
    appendOperands({ " ", rd, ",", rs1, ",", rs2, ",", rs3 });
}

util::Expected_u<const InstLayout *> R4Type::Assembly()
{
    const auto &info  = LookupIdxAndInfo();
    const auto PAYLOAD= KEY_PAYLOAD(info.keyID_);

    Layout_.R4.opc= info.opcode_;
    Layout_.R4.fmt= PAYLOAD & 0x3U;
    Layout_.R4.rm = G_RM_DYN;

    if(InstAssembly_.size() >= 5U) {
        if(auto rd= isa::LookupFregIdx(InstAssembly_.at(1))) Layout_.R4.rd= *rd;
        if(auto rs1= isa::LookupFregIdx(InstAssembly_.at(2))) Layout_.R4.rs1= *rs1;
        if(auto rs2= isa::LookupFregIdx(InstAssembly_.at(3))) Layout_.R4.rs2= *rs2;
        if(auto rs3= isa::LookupFregIdx(InstAssembly_.at(4))) Layout_.R4.rs3= *rs3;
    }

    mnemonicHelper();
    return &Layout_;
}

auto R4Type::calculateKeyID() -> KeyT
{
    return Key_= R4_K(Layout_.R4.fmt, Layout_.R4.opc);
}

auto R4Type::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
