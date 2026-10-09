#include <print>

#include "Core/BType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

namespace {

int32_t decodeBImm13(const InstLayout &l)
{
    const int32_t S_IMM= (l.B.immC << 12) | (l.B.immB << 11) | (l.B.imm5tA << 5) | (l.B.imm1t4 << 1);
    return (S_IMM << 19) >> 19;
}

void encodeBImm13(InstLayout &l, int32_t imm)
{
    const int32_t S_IMM= imm & 0x1FFF;

    l.B.imm1t4= S_IMM >> 1;
    l.B.immB  = S_IMM >> 11;
    l.B.imm5tA= S_IMM >> 5;
    l.B.immC  = S_IMM >> 12;
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
    setBitsField({ Layout_.B.opc,
                   Layout_.B.immB,
                   Layout_.B.imm1t4,
                   Layout_.B.fct3,
                   Layout_.B.rs1,
                   Layout_.B.rs2,
                   Layout_.B.imm5tA,
                   Layout_.B.immC });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrs2: {}\nimm: {}",
                 GetInstOpcode(),
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

const InstLayout &BType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.B.opc= info.opcode_;
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
    FunctKey_= static_cast<KeyT>((Layout_.B.opc << 8) | (Layout_.B.fct3 & 7U));
    return FunctKey_;
}

const BiLookupTable<IBaseInstType::KeyT> *BType::buildTable()
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
