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
    const static inline std::flat_map<OpcMap, InstFormat> G_OPCODE_2_FORMAT {
        std::sorted_unique, // data is already 'sorted' and has 'no duplicate' elements, skip the internal sorting and deduplication steps.
        {
            { OpcMap::LOAD, InstFormat::I },      // I-type: lb/lh/lw/ld/lwu/ldu/...
            { OpcMap::LOAD_FP, InstFormat::I },   // I-type: flw/fld/flq / LOAD-FP
            { OpcMap::MISC_MEM, InstFormat::I },  // I-type: fence* / lq (RV128I)
            { OpcMap::OP_IMM, InstFormat::I },    // I-type: addi/slli/...
            { OpcMap::AUIPC, InstFormat::U },     // U-type: auipc
            { OpcMap::OP_IMM_32, InstFormat::I }, // I-type: addiw/slliw/... / OP-IMM-32 (RV64I)
            { OpcMap::STORE, InstFormat::S },     // S-type: sb/sh/sw/sd/sq
            { OpcMap::STORE_FP, InstFormat::S },  // S-type: fsw/fsd/fsq / STORE-FP
            { OpcMap::AMO, InstFormat::R },       // R-type: lr/sc/amo... / AMO (RV32A/64A)
            { OpcMap::OP, InstFormat::R },        // R-type: add/sub/... / OP
            { OpcMap::LUI, InstFormat::U },       // U-type: lui
            { OpcMap::OP_32, InstFormat::R },     // R-type: addw/subw/... / OP-32 (RV64I)
            { OpcMap::MADD, InstFormat::R4 },     // R4: fmadd.*
            { OpcMap::MSUB, InstFormat::R4 },     // R4: fmsub.*
            { OpcMap::NMSUB, InstFormat::R4 },    // R4: fnmsub.*
            { OpcMap::NMADD, InstFormat::R4 },    // R4: fnmadd.*
            { OpcMap::OP_FP, InstFormat::R },     // R-type: fadd/fmul/... / OP-FP
            { OpcMap::OP_IMM_64, InstFormat::I }, // I-type: addid/sllid/... / OP-IMM-64 (RV128I)
            { OpcMap::BRANCH, InstFormat::B },    // B-type: beq/bne etc.
            { OpcMap::JALR, InstFormat::I },      // I-type: jalr
            { OpcMap::JAL, InstFormat::J },       // J-type: jal
            { OpcMap::SYSTEM, InstFormat::I },    // I-type: ecall / ebreak
            { OpcMap::OP_64, InstFormat::R },     // R-type: addd/subd/... / OP-64 (RV128I)

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
