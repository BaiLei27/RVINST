#pragma once

#include <vector>
#include <initializer_list>

#include "ISA/InstFormat.hh"
#include "Util/BiLookupTable.hpp"
#include "Util/ErrorHandle.hh"

class IBaseInstType {
public:
    using KeyT= uint32_t; // NOLINT

    struct InstInfo {
        std::string_view name_;
        std::string_view XLEN_; // NOLINT
        KeyT keyID_ {};
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
    KeyT Key_ { 0xffff }; // (payload << 7) | opcode
    bool HasSetABI_ {};

public:
    IBaseInstType(InstFormat format, uint32_t inst, bool hasSetABI);
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
    IBaseInstType(InstFormat format, std::vector<std::string> instAssembly, bool hasSetABI);
    virtual ~IBaseInstType()= default;

public:
    const InstLayout &GetInstLayout() const noexcept;
    const std::vector<std::string> &GetInstAssembly() const noexcept;
    const std::vector<uint32_t> &GetInstBitsField() const noexcept;

    void SetInstAssembly(std::vector<std::string> assembly);
    void SetFormat(InstFormat format) noexcept;

    [[nodiscard]] uint8_t GetInstOpcode() const noexcept;
    [[nodiscard]] KeyT GetKey() const noexcept;
    [[nodiscard]] InstFormat GetInstFormat() const noexcept;
    [[nodiscard]] const BiLookupTable<KeyT>::NameInfo &LookupNameAndInfo() const;
    [[nodiscard]] BiLookupTable<KeyT>::IndexInfo LookupIdxAndInfo() const;
    [[nodiscard]] const std::vector<std::string> &Disassembly();

    virtual void Parse()                                                 = 0;
    [[nodiscard]] virtual util::Expected_u<const InstLayout *> Assembly()= 0;

    // Unified keyID: (payload << 7) | (opcode & 0x7F). Defined below for Type table constexpr.
    [[nodiscard]] static constexpr KeyT PACK_KEY(KeyT payload, uint8_t opc) noexcept { return payload << 7 | opc & 0x7FU; }

    [[nodiscard]] static constexpr KeyT KEY_PAYLOAD(KeyT key) noexcept { return key >> 7; }

protected:
    virtual const BiLookupTable<KeyT> *buildTable()= 0;
    virtual KeyT calculateKeyID()                  = 0;
    virtual void mnemonicHelper()                  = 0;

    void init();
    void appendOperands(std::initializer_list<std::string_view> regs);
    void setBitsField(std::initializer_list<uint32_t> bits);
    void setBitsField(std::span<const uint32_t> bits);

    [[nodiscard]] static const BiLookupTable<KeyT> *
        makeLookupTable(std::span<const InstInfo> entries, std::string_view baseURL);
};

// Date:25/12/19/16:23
