#include <print>

#include "Core/IType.hh"
#include "ISA/Regs.hpp"
#include "Util/InputParse.hpp"

IType::IType(uint32_t inst, InstFormat format, bool hasSetABI)
    : IBaseInstType(inst, format, hasSetABI)
{
    init();
}

IType::IType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI)
    : IBaseInstType(std::move(instAssembly), format, hasSetABI)
{
    init();
}

void IType::Parse()
{
    setBitsField({ Layout_.I.opc,
                   Layout_.I.rd,
                   Layout_.I.fct3,
                   Layout_.I.rs1,
                   Layout_.I.imm0tB });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrd: {}\nimm: {}",
                 GetInstOpcode(),
                 Layout_.entity_,
                 +Layout_.I.fct3,
                 +Layout_.I.rs1,
                 +Layout_.I.rd,
                 static_cast<int16_t>(Layout_.I.imm0tB));
#endif
}

void IType::mnemonicHelper()
{
    const uint32_t OPC= Layout_.I.opc;
    if(OpcMap::SYSTEM == OPC) {
        if(InstAssembly_.size() > 1) InstAssembly_.resize(1);
        return;
    }

    std::string immStr= std::to_string(Layout_.I.imm0tB);
    if(ITypeKey::MEM == OPC) {
        auto z= isa::LOOKUP_REG_NAME(0, HasSetABI_);
        appendOperands({ " ", z, ",", z, ",", immStr });
        return;
    }

    auto rd = isa::LOOKUP_REG_NAME(Layout_.I.rd, HasSetABI_);
    auto rs1= isa::LOOKUP_REG_NAME(Layout_.I.rs1, HasSetABI_);
    appendOperands({ " ", rd, ",", rs1, ",", immStr });
}

const InstLayout &IType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.I.opc     = info.opcode_;
    const uint16_t KEY= info.funct_;

    switch(info.opcode_) {
    case OpcMap::OP_IMM: {
        Layout_.I.fct3  = KEY & 7;
        Layout_.I.imm0tB= (KEY << 2) & 0xFE0;
    } break;
    case OpcMap::SYSTEM: {
        Layout_.I.rd    = 0;
        Layout_.I.rs1   = 0;
        Layout_.I.fct3  = 0;
        Layout_.I.imm0tB= KEY & 0xFFF;
    } break;
    default: {
        Layout_.I.fct3  = KEY & 7;
        Layout_.I.imm0tB= 0;
        break;
    }
    }

    if(InstAssembly_.size() >= 4) {
        if(auto rdOpt= isa::LOOKUP_REG_IDX(InstAssembly_.at(1))) {
            Layout_.I.rd= *rdOpt;
        }
        if(auto rs1Opt= isa::LOOKUP_REG_IDX(InstAssembly_.at(2))) {
            Layout_.I.rs1= *rs1Opt;
        }
        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(3), imm)) {
            if(OpcMap::OP_IMM == info.opcode_) {
                if(1 == Layout_.I.fct3 || 5 == Layout_.I.fct3) {
                    Layout_.I.imm0tB= (Layout_.I.imm0tB & 0xFE0U) | (imm & 0x1F);
                } else {
                    Layout_.I.imm0tB= imm & 0xFFF;
                }
            } else if(OpcMap::SYSTEM != info.opcode_) {
                Layout_.I.imm0tB= imm & 0xFFF;
            }
        }
    }

    mnemonicHelper();
    return Layout_;
}

IBaseInstType::KeyT IType::calculateFunctKey()
{
    switch(Layout_.I.opc) {
    case OpcMap::OP_IMM:
        FunctKey_= ((Layout_.I.imm0tB >> 5) << 3) | Layout_.I.fct3;
        break;
    case OpcMap::SYSTEM:
        FunctKey_= SYS_IMM(Layout_.I.imm0tB);
        break;
    default:
        FunctKey_= Layout_.I.fct3;
        break;
    }
    return FunctKey_;
}

const BiLookupTable<IBaseInstType::KeyT> *IType::buildTable()
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
