#include "Core/SType.hh"
#include "ISA/Regs.hh"
#include "Util/InputParse.hh"

namespace {

int32_t decodeSImm12(const InstLayout &l)
{
    const int32_t S_IMM= (l.S.imm5tB << 5) | l.S.imm0t4;
    return (S_IMM << 20) >> 20;
}

void encodeSImm12(InstLayout &l, int32_t imm)
{
    const uint32_t S_IMM= imm & 0xFFF;

    l.S.imm0t4= S_IMM;
    l.S.imm5tB= S_IMM >> 5;
}

} // namespace

SType::SType(InstFormat format, uint32_t inst, bool hasSetABI)
    : IBaseInstType(format, inst, hasSetABI)
{
    init();
}

SType::SType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI)
    : IBaseInstType(format, std::move(instAssembly), hasSetABI)
{
    init();
}

void SType::Parse()
{
    setBitsField({ Layout_.S.opc,
                   Layout_.S.imm0t4,
                   Layout_.S.fct3,
                   Layout_.S.rs1,
                   Layout_.S.rs2,
                   Layout_.S.imm5tB });
#ifdef DEBUG_
    std::println("opcode: 0x{:x}\nHexadecimal: 0x{:x}\nfunct3: {}\nrs1: {}\nrs2: {}\nimm: {}",
                 InstOpcode(),
                 Layout_.entity_,
                 +Layout_.S.fct3,
                 +Layout_.S.rs1,
                 +Layout_.S.rs2,
                 decodeSImm12(Layout_));
#endif
}

void SType::mnemonicHelper()
{
    auto rs2   = OpcMap::STORE_FP == Layout_.S.opc ? isa::LookupFregName(Layout_.S.rs2, HasSetABI_)
                                                   : isa::LookupRegName(Layout_.S.rs2, HasSetABI_);
    auto rs1   = isa::LookupRegName(Layout_.S.rs1, HasSetABI_);
    auto immStr= std::to_string(decodeSImm12(Layout_));

    appendOperands({ rs2, ",", immStr, "(", rs1, ")" });
}

util::Expected_u<const InstLayout *> SType::Assembly()
{
    const auto &info= LookupIdxAndInfo();

    Layout_.S.opc = info.opcode_;
    Layout_.S.fct3= KEY_PAYLOAD(info.keyID_);

    auto setRs2= [&](std::string_view tok) {
        if(OpcMap::STORE_FP == Layout_.S.opc) {
            if(auto rs2Opt= isa::LookupFregIdx(tok)) Layout_.S.rs2= *rs2Opt;
        } else if(auto rs2Opt= isa::LookupRegIdx(tok)) {
            Layout_.S.rs2= *rs2Opt;
        }
    };

    if(InstAssembly_.size() >= 4U) {
        setRs2(InstAssembly_.at(1));
        int32_t imm {};
        if(util::ParseInt(InstAssembly_.at(2), imm)) {
            encodeSImm12(Layout_, imm);
        }
        if(auto rs1Opt= isa::LookupRegIdx(InstAssembly_.at(3))) {
            Layout_.S.rs1= *rs1Opt;
        }
    } else if(InstAssembly_.size() >= 3U) {
        setRs2(InstAssembly_.at(1));
        int32_t imm {};
        std::string baseReg;
        if(util::ParseMemOperand(InstAssembly_.at(2), imm, baseReg)) {
            encodeSImm12(Layout_, imm);
            if(auto rs1Opt= isa::LookupRegIdx(baseReg)) {
                Layout_.S.rs1= *rs1Opt;
            }
        }
    }

    mnemonicHelper();
    return &Layout_;
}

auto SType::calculateKeyID() -> KeyT
{
    return Key_= OP_K(Layout_.S.fct3, Layout_.S.opc);
}

auto SType::buildTable() -> const BiLookupTable<KeyT> *
{
    static const auto *s_TABLE= makeLookupTable(G_INST_TABLE, G_ManualBaseURL);
    return s_TABLE;
}
