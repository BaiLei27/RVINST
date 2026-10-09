#pragma once

#include <vector>
#include <initializer_list>

#include "ISA/InstFormat.hh"
#include "Util/BiLookupTable.hpp"

enum OpcMap : uint8_t { // RISC-V base opcode map, inst[1:0] === 0b11
    // inst[6:5] = 00
    LOAD     = 0x03, // 00_000_11
    LOAD_FP  = 0x07, // 00_001_11
    CUSTOM_0 = 0x0B, // 00_010_11
    MISC_MEM = 0x0F, // 00_011_11
    OP_IMM   = 0x13, // 00_100_11
    AUIPC    = 0x17, // 00_101_11
    OP_IMM_32= 0x1B, // 00_110_11 (RV64 only)
    RESERVE_1= 0x1F, // 00_111_11

    // inst[6:5] = 01
    STORE    = 0x23, // 01_000_11
    STORE_FP = 0x27, // 01_001_11
    CUSTOM_1 = 0x2B, // 01_010_11
    AMO      = 0x2F, // 01_011_11
    OP       = 0x33, // 01_100_11
    LUI      = 0x37, // 01_101_11
    OP_32    = 0x3B, // 01_110_11 (RV64 only)
    RESERVE_2= 0x3F, // 01_111_11

    // inst[6:5] = 10
    MADD     = 0x43, // 10_000_11
    MSUB     = 0x47, // 10_001_11
    NMSUB    = 0x4B, // 10_010_11
    NMADD    = 0x4F, // 10_011_11
    OP_FP    = 0x53, // 10_100_11
    OP_V     = 0x57, // 10_101_11
    CUSTOM_2 = 0x5B, // 10_110_11
    RESERVE_3= 0x5F, // 10_111_11

    // inst[6:5] = 11
    BRANCH   = 0x63, // 11_000_11
    JALR     = 0x67, // 11_001_11
    RESERVE_4= 0x6B, // 11_010_11
    JAL      = 0x6F, // 11_011_11
    SYSTEM   = 0x73, // 11_100_11
    OP_VE    = 0x77, // 11_101_11
    CUSTOM_3 = 0x7B, // 11_110_11
    RESERVE_5= 0x7F, // 11_111_11
};

class IBaseInstType {
public:
    using KeyT= uint16_t; // NOLINT

    struct InstInfo {
        std::string_view name_;
        std::string_view XLEN_; // NOLINT
        KeyT funct_ {};
        uint16_t opcode_ {};
    };

    static constexpr std::string_view G_ManualBaseURL { "https://riscv.github.io/riscv-unified-db/manual/html/isa/isa_20240411/insts/" };

protected:
    std::optional<BiLookupTable<KeyT>::IndexInfo> FunctOpcAndXlenCache_;
    std::optional<BiLookupTable<KeyT>::NameInfo> NameAndXlenCache_;
    std::vector<std::string> InstAssembly_;
    std::vector<uint32_t> InstBitsField_;
    InstFormat Format_ { InstFormat::UNKNOWN };
    InstLayout Layout_;
    const BiLookupTable<KeyT> *InstTable_ {};
    KeyT FunctKey_ { 0xffff }; // misc | funct3
    bool HasSetABI_ {};

public:
    IBaseInstType(uint32_t inst, InstFormat format, bool hasSetABI);
    /**
     * @brief Construct a new IBaseInstType object
     * @param instAssembly , why rvalues ?
     * 1. Store objects in constructors or setters.
     * 2. Want to support both lvalues ​​and rvalues.
     *    2.1 For rvalues ​​(temporary objects), the entire process involves only a move operation (a lightweight operation),
     *        avoiding unnecessary deep copies.
     *    2.2 For lvalues, a necessary copy is made before the move operation is performed on the member.
     *        Only unavoidable copy operations are performed.
     * 3. Types with low construction overhead (e.g., string/vector).
     * 4. Don't want to write two overload statements.
     * @param format
     * @param hasSetABI
     */
    IBaseInstType(std::vector<std::string> instAssembly, InstFormat format, bool hasSetABI);
    virtual ~IBaseInstType()= default;

public:
    const InstLayout &GetInstLayout() const noexcept;
    const std::vector<std::string> &GetInstAssembly() const noexcept;
    const std::vector<uint32_t> &GetInstBitsField() const noexcept;
    [[nodiscard]] uint16_t GetInstOpcode() const noexcept;
    [[nodiscard]] KeyT GetInstFunctKey() const noexcept;
    [[nodiscard]] InstFormat GetInstFormat() const noexcept;

    void SetInstAssembly(std::vector<std::string> assembly);
    void SetFormat(InstFormat format) noexcept;

    [[nodiscard]] const BiLookupTable<KeyT>::NameInfo &LookupNameAndInfo() const;
    [[nodiscard]] BiLookupTable<KeyT>::IndexInfo LookupIdxAndInfo() const;
    [[nodiscard]] const std::vector<std::string> &Disassembly();

    virtual void Parse()                = 0;
    virtual const InstLayout &Assembly()= 0;

protected:
    virtual const BiLookupTable<KeyT> *buildTable()= 0;
    virtual KeyT calculateFunctKey()               = 0;
    virtual void mnemonicHelper()                  = 0;

    void init();
    void appendOperands(std::initializer_list<std::string_view> regs);
    void setBitsField(std::initializer_list<uint32_t> bits);
    void setBitsField(std::span<const uint32_t> bits);

    [[nodiscard]] static const BiLookupTable<KeyT> *
        makeLookupTable(std::span<const InstInfo> entries, std::string_view baseURL);
};

// Date:25/12/19/16:23
