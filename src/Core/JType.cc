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

    if(const uint32_t SIGNED_BIT= 1U << 20; 0U != (sImm & SIGNED_BIT)) {
        sImm|= ~0x1FFFFF;
    }
    return sImm;
}

void encodeJImm(InstLayout &l, int32_t imm)
{
    const int32_t S_IMM= imm & 0x1FFFFF;
    l.J.imm14          = S_IMM >> 20;
    l.J.imm1tA         = S_IMM >> 1;
    l.J.immB           = S_IMM >> 11;
    l.J.immCt13        = S_IMM >> 12;
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
    setBitsField({ Layout_.J.opc,
                   Layout_.J.rd,
                   Layout_.J.immCt13,
                   Layout_.J.immB,
                   Layout_.J.imm1tA,
                   Layout_.J.imm14 });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nrd: {}\nimm: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.J.rd,
                 decodeJImm(Layout_));
#endif
}

void JType::mnemonicHelper()
{
    auto rd           = isa::LOOKUP_REG_NAME(Layout_.J.rd, HasSetABI_);
    std::string immStr= std::to_string(decodeJImm(Layout_));
    appendOperands({ " ", rd, ",", immStr });
}

const InstLayout &JType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.J.opc= info.opcode_;

    if(InstAssembly_.size() >= 3) {
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

const BiLookupTable<IBaseInstType::KeyT> *JType::buildTable()
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
