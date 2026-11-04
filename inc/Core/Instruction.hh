#pragma once
#include <bitset>
#include <sstream>
#include <memory>

#include "Core/IBaseInstType.hh"
#include "Util/ErrorHandle.hh"

class Instruction {
private:
    std::unique_ptr<IBaseInstType> Type_;
    std::stringstream Disassembly_ { "unimp" };
    std::string Format_ { "UNKNOW" }, XLEN_ { "UNDEF" }, Manual_ { "Not available" }, Name_ { "unimp" };
    std::bitset<32> BitField_; // 32-bit bit field storage

public:
    explicit Instruction(uint32_t inst, bool hasSetABI= false);
    explicit Instruction(std::string_view assembly, bool hasSetABI= false);
    // explicit Instruction(std::stringstream assembly, bool hasSetABI= false);
    Instruction(const Instruction &)            = delete;
    Instruction &operator= (const Instruction &)= delete;
    Instruction(Instruction &&that) noexcept;
    Instruction &operator= (Instruction &&that) noexcept;
    virtual ~Instruction()= default;

    explicit operator std::string() const;
    explicit operator uint32_t() const;

public:
    [[nodiscard]] const IBaseInstType *TypePtr() const;
    [[nodiscard]] const IBaseInstType &Type() const;
    [[nodiscard]] const std::bitset<32> &BitField() const;

    [[nodiscard]] std::string HexStr() const;
    [[nodiscard]] std::string BinStr() const;
    [[nodiscard]] std::string_view XLEN() const;
    [[nodiscard]] std::string_view Manual() const;
    [[nodiscard]] std::string_view Name() const;
    [[nodiscard]] std::string_view Format() const noexcept;

    [[nodiscard]] util::Expected_u<void> Decode();
    void ShowInfo() const;

private:
    // static std::vector<std::string> handleAssembly(std::string &assembly);
    void resetStream();
};

// Date:25/12/22/14:56
