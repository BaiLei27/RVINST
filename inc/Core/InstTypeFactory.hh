#pragma once

#include <memory>
#include <utility>
#include <flat_map>

#include "Core/IBaseInstType.hh"

class InstTypeFactory {
private:
    /**
     * @brief
     * Opcode -> instruction format. const 'std::flat_map': contiguous
     * vector-backed storage with O(log n) binary-search lookup instead of a
     * hash table (see CreateType), and no runtime initialization at all.
     * Entries are kept sorted by opcode so the sorted_unique constructor skips the (const) sort step.
     */

    // clang-format off
    const static inline std::flat_map<uint16_t, InstFormat> G_OPCODE_2_FORMAT {
        std::sorted_unique, // data is already 'sorted' and has 'no duplicate' elements, skip the internal sorting and deduplication steps.
        {
            { 0x03, InstFormat::I }, // I-type: lb/lh/lw/...
            { 0x0F, InstFormat::I }, // I-type: fence / fence.i
            { 0x13, InstFormat::I }, // I-type: addi/slli/...
            { 0x17, InstFormat::U }, // U-type: auipc
            { 0x23, InstFormat::S }, // S-type: sw/sb etc.
            { 0x33, InstFormat::R }, // R-type: add/sub/sll/slt etc.
            { 0x37, InstFormat::U }, // U-type: lui
            { 0x63, InstFormat::B }, // B-type: beq/bne etc.
            { 0x67, InstFormat::I }, // I-type: jalr
            { 0x6F, InstFormat::J }, // J-type: jal
            { 0x73, InstFormat::I }, // I-type: ecall / ebreak

            // clang-format on
        }
    };

public:
    /**
     * @brief
     * name -> (format, opcode): std::flat_map keeps keys sorted in contiguous
     * vector-backed storage, so lookup is O(log n) binary search with good
     * cache locality. Built once then treated as read-only (see 'getName2FormatOpcode()').
     */
    using name2FormatOpcode_u= std::flat_map<std::string_view, std::pair<InstFormat, uint16_t>>;

    static std::unique_ptr<IBaseInstType> CreateType(uint32_t inst, bool hasSetABI= false);
    static std::unique_ptr<IBaseInstType> CreateType(std::vector<std::string> instAssembly, bool hasSetABI= false);

    /**
     * @brief Lightweight lookup shared by CreateType(): true when 'name' (case-insensitive)
     * @param name
     * @return  is a known mnemonic. No instruction object is constructed.
     * @return false
     */
    [[nodiscard]] static bool IsKnownMnemonic(std::string_view name) noexcept;

private:
    [[nodiscard]] static std::optional<std::pair<InstFormat, uint16_t>> matchInstName(std::string_view instName);
    static const name2FormatOpcode_u &getName2FormatOpcode();
};
