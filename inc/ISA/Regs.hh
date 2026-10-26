#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

// Fixed-size, compile-time mapping of register index -> (name, ABI)
namespace isa {

constexpr uint16_t G_REG_NUMBER= 32;

using strPair_u   = std::pair<std::string_view, std::string_view>;
using RegFileTable= std::array<strPair_u, G_REG_NUMBER>;

constexpr RegFileTable G_INDEX_REGS_ABI= {
    { { "x0", "zero" },
     { "x1", "ra" },
     { "x2", "sp" },
     { "x3", "gp" },
     { "x4", "tp" },
     { "x5", "t0" },
     { "x6", "t1" },
     { "x7", "t2" },
     { "x8", "s0/fp" },
     { "x9", "s1" },
     { "x10", "a0" },
     { "x11", "a1" },
     { "x12", "a2" },
     { "x13", "a3" },
     { "x14", "a4" },
     { "x15", "a5" },
     { "x16", "a6" },
     { "x17", "a7" },
     { "x18", "s2" },
     { "x19", "s3" },
     { "x20", "s4" },
     { "x21", "s5" },
     { "x22", "s6" },
     { "x23", "s7" },
     { "x24", "s8" },
     { "x25", "s9" },
     { "x26", "s10" },
     { "x27", "s11" },
     { "x28", "t3" },
     { "x29", "t4" },
     { "x30", "t5" },
     { "x31", "t6" } }
};

static_assert(G_REG_NUMBER == G_INDEX_REGS_ABI.size());

[[nodiscard]] std::optional<uint16_t> LookupRegIdx(std::string_view target);
[[nodiscard]] std::optional<strPair_u> ParseRegName(size_t idx) noexcept;
[[nodiscard]] std::string_view LookupRegName(size_t regIdx,
                                             bool hasSetABI           = false,
                                             std::string_view fallback= "?") noexcept;

// Floating-point register file (f0–f31) and ABI aliases (ft*/fs*/fa*).
constexpr RegFileTable G_INDEX_FREGS_ABI= {
    { { "f0", "ft0" },
     { "f1", "ft1" },
     { "f2", "ft2" },
     { "f3", "ft3" },
     { "f4", "ft4" },
     { "f5", "ft5" },
     { "f6", "ft6" },
     { "f7", "ft7" },
     { "f8", "fs0" },
     { "f9", "fs1" },
     { "f10", "fa0" },
     { "f11", "fa1" },
     { "f12", "fa2" },
     { "f13", "fa3" },
     { "f14", "fa4" },
     { "f15", "fa5" },
     { "f16", "fa6" },
     { "f17", "fa7" },
     { "f18", "fs2" },
     { "f19", "fs3" },
     { "f20", "fs4" },
     { "f21", "fs5" },
     { "f22", "fs6" },
     { "f23", "fs7" },
     { "f24", "fs8" },
     { "f25", "fs9" },
     { "f26", "fs10" },
     { "f27", "fs11" },
     { "f28", "ft8" },
     { "f29", "ft9" },
     { "f30", "ft10" },
     { "f31", "ft11" } }
};

static_assert(G_REG_NUMBER == G_INDEX_FREGS_ABI.size());

[[nodiscard]] std::optional<uint16_t> LookupFregIdx(std::string_view target);
[[nodiscard]] std::optional<strPair_u> ParseFregName(size_t idx) noexcept;
[[nodiscard]] std::string_view LookupFregName(size_t regIdx,
                                              bool hasSetABI           = false,
                                              std::string_view fallback= "?") noexcept;

// Unprivileged CSRs for RV32/64G (FP + Zicntr). Sorted by num_ for binary search.
struct CsrEntry {
    uint16_t num_ {};
    std::string_view name_;
};

constexpr std::array<CsrEntry, 9> G_UCSR_TABLE {
    {
     { .num_= 0x001, .name_= "fflags" },
     { .num_= 0x002, .name_= "frm" },
     { .num_= 0x003, .name_= "fcsr" },
     { .num_= 0xC00, .name_= "cycle" },
     { .num_= 0xC01, .name_= "time" },
     { .num_= 0xC02, .name_= "instret" },
     { .num_= 0xC80, .name_= "cycleh" },
     { .num_= 0xC81, .name_= "timeh" },
     { .num_= 0xC82, .name_= "instreth" },
     }
};

[[nodiscard]] std::optional<std::string_view> LookupCSRName(uint16_t num) noexcept;
[[nodiscard]] std::optional<uint16_t> LookupCSRIdx(std::string_view name);

} // namespace isa
