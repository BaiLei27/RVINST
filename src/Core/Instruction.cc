#include <algorithm>
#include <format>
#include <print>

#include "Core/Instruction.hh"
#include "Core/InstTypeFactory.hh"
#include "ISA/InstFormat.hh"

Instruction::Instruction(uint32_t inst, bool hasSetABI)
    : Type_(InstTypeFactory::CreateType(inst, hasSetABI)),
      BitField_(inst)
{
    if(Type_) {
        const auto &[MAN_URL, XLEN, _1]= Type_->LookupNameAndInfo();

        XLEN_  = XLEN;
        Manual_= MAN_URL;
        Format_= GetFormat();
    }
}

Instruction::Instruction(std::string_view assembly, bool hasSetABI)
{
    std::string s(assembly);
    std::ranges::replace(s, ',', ' ');
    std::istringstream tmp(s);

    std::vector<std::string> parts;
    for(std::string token; tmp >> token;) {
        parts.emplace_back(std::move(token));
    }

    Type_= InstTypeFactory::CreateType(std::move(parts), hasSetABI);

    if(Type_) {
        const auto &[MAN_URL, XLEN, _1, opc]= Type_->LookupIdxAndInfo();

        XLEN_  = XLEN;
        Manual_= MAN_URL;
        Format_= GetFormat();
    }
}

Instruction::Instruction(Instruction &&that) noexcept
    : Type_(std::move(that.Type_)),
      Disassembly_(std::move(that.Disassembly_)),
      XLEN_(that.XLEN_),
      Manual_(that.Manual_),
      Format_(that.Format_),
      BitField_(that.BitField_)
{
    that.XLEN_  = "UNDEF";
    that.Format_= "UNKNOW";
    that.Manual_= "Not available";
}

Instruction &Instruction::operator= (Instruction &&that) noexcept
{
    if(&that != this) {
        Type_       = std::move(that.Type_);
        Disassembly_= std::move(that.Disassembly_);
        XLEN_       = that.XLEN_;
        Manual_     = that.Manual_;
        BitField_   = that.BitField_;

        that.XLEN_  = "UNDEF";
        that.Format_= "UNKNOW";
        that.Manual_= "Not available";
    }
    return *this;
}

Instruction::operator std::string() const { return Disassembly_.str(); }

Instruction::operator uint32_t() const { return BitField_.to_ulong(); }

const IBaseInstType *Instruction::GetTypePtr() const { return Type_.get(); }

const IBaseInstType &Instruction::GetType() const { return *Type_; }

const std::bitset<32> &Instruction::GetBitField() const { return BitField_; }

std::string Instruction::GetHexStr() const
{
    return std::format("{:08X}", static_cast<unsigned>(BitField_.to_ulong()));
}

std::string Instruction::GetBinStr() const { return BitField_.to_string(); }

std::string_view Instruction::GetXLEN() const { return XLEN_; }

std::string_view Instruction::GetManual() const { return Manual_; }

std::string_view Instruction::GetFormat() const noexcept
{
    if(!Type_) return "UNKNOW";

    return InstFormatName(Type_->GetInstFormat());
}

bool Instruction::Decode()
{
    if(Type_) {
        if(BitField_.none()) {
            std::bitset<32> tmp(Type_->Assembly());
            BitField_= std::move(tmp);
            resetStream();

        } else {
            Type_->Disassembly();
        }
        Type_->Parse();

        const auto &v= Type_->GetInstAssembly();
        for(const auto &e: v) {
            Disassembly_ << e;
            // std::println("{}", Disassembly_.str());
        }
        return true;
    }

    std::println(stderr, "unimp instruction: 0x{:08X}", BitField_.to_ulong());

    return false;
}

void Instruction::ShowInfo() const
{
    std::println("BitField: {}\nAssembly: {}\nFormat: {}\nArch: {}\nManual: {}",
                 BitField_.to_string(),
                 Disassembly_.str(),
                 Format_,
                 XLEN_,
                 Manual_);
}

void Instruction::resetStream()
{
    Disassembly_.str("");
    Disassembly_.clear();
    // Disassembly_.seekg(0);
    // Disassembly_.seekp(0);
}
