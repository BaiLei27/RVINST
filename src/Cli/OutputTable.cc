#include "Cli/OutputTable.hh"

#include <algorithm>
#include <cstdio>
#include <format>
#include <iostream>
#include <iterator>
#include <print>
#include <streambuf>
#include <string>
#include <unistd.h>

#include "Core/IBaseInstType.hh"
#include "Core/Instruction.hh"
#include "ISA/InstFormat.hh"
#include "ISA/Regs.hpp"

namespace cli {

namespace {
    class NullBuf: public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
    };

    constexpr auto G_C_RESET  = "\033[0m";
    constexpr auto G_C_BOLD   = "\033[1m";
    constexpr auto G_C_CYAN   = "\033[36m";
    constexpr auto G_C_GREEN  = "\033[32m";
    constexpr auto G_C_YELLOW = "\033[33m";
    constexpr auto G_C_MAGENTA= "\033[35m";

    bool isRegField(std::string_view name)
    {
        return name == "rd" || name == "rs1" || name == "rs2";
    }

    std::string padRight(std::string_view s, size_t width)
    {
        return s.size() >= width ? std::string(s) : std::string(s) + std::string(width - s.size(), ' ');
    }

} // namespace

CoutSuppressor::CoutSuppressor()
{
    static NullBuf s_nullBuf;
    OldBuf_= std::cout.rdbuf(&s_nullBuf);
}

CoutSuppressor::~CoutSuppressor()
{
    if(OldBuf_) std::cout.rdbuf(OldBuf_);
}

OutputMode detectOutputMode(bool forceJson)
{
    if(forceJson) return OutputMode::JSON;

    if(isatty(fileno(stdout)) != 0) return OutputMode::COLORED;

    return OutputMode::PLAIN;
}

OutputFormatter::OutputFormatter(OutputMode mode): Mode_(mode) { }

OutputMode OutputFormatter::Mode() const noexcept { return Mode_; }

std::string OutputFormatter::paint(std::string_view text, std::string_view code) const
{
    if(OutputMode::COLORED == Mode_) return std::string(code) + std::string(text) + G_C_RESET;

    return std::string(text);
}

std::string OutputFormatter::bold(std::string_view t) const { return paint(t, G_C_BOLD); }

std::string OutputFormatter::cyan(std::string_view t) const { return paint(t, G_C_CYAN); }

std::string OutputFormatter::green(std::string_view t) const { return paint(t, G_C_GREEN); }

std::string OutputFormatter::yellow(std::string_view t) const { return paint(t, G_C_YELLOW); }

std::string OutputFormatter::magenta(std::string_view t) const { return paint(t, G_C_MAGENTA); }

void OutputFormatter::PrintInstruction(const Instruction &inst, std::string_view input)
{
    if(OutputMode::JSON == Mode_) {
        printInstructionJSON(inst, input);
        return;
    }

    constexpr size_t LABEL_W= 12;
    std::string buf;
    buf.reserve(512);
    auto out= std::back_inserter(buf);

    std::format_to(out, "\n  {}\n", cyan("Instruction"));
    auto kv= [&](std::string_view label, std::string_view value) {
        std::format_to(out, "  {}{}\n", bold(padRight(label, LABEL_W)), value);
    };

    kv("Input", input);
    kv("Binary", inst.GetBinStr());
    kv("Assembly", std::string(inst));
    kv("Hex", std::string("0x") + inst.GetHexStr());
    kv("Format", inst.GetFormat());
    kv("ISA", inst.GetXLEN());
    kv("Manual", inst.GetManual());

    std::print("{}", buf);
    printFieldTable(inst);
    printBitFieldDiagram(inst);
}

void OutputFormatter::printFieldTable(const Instruction &inst)
{
    const auto *pTypePtr= inst.GetTypePtr();
    if(!pTypePtr) return;

    const auto &bitsField= pTypePtr->GetInstBitsField();
    if(bitsField.empty()) return;

    InstFormat fmt  = pTypePtr->GetInstFormat();
    const auto &meta= GetInstFields(fmt);
    if(meta.empty()) return;

    const size_t TOTAL     = meta.size();
    constexpr size_t NAME_W= 12, BITS_W= 12, VAL_W= 10, DETAIL_W= 10;
    const size_t SEP_W= NAME_W + BITS_W + VAL_W + DETAIL_W + 2;

    std::string buf;
    buf.reserve(128 + TOTAL * 64);
    auto out= std::back_inserter(buf);

    std::format_to(out, "\n  {}\n", cyan("Fields"));
    std::format_to(out,
                   "  {}{}{}{}\n  {}\n",
                   bold(padRight("Field", NAME_W)),
                   bold(padRight("Bits", BITS_W)),
                   bold(padRight("Value", VAL_W)),
                   bold("Detail"),
                   std::string(SEP_W, '-'));

    for(size_t i= 0; i < TOTAL; ++i) {
        const auto &fm     = meta[i];
        const size_t BF_IDX= TOTAL - 1 - i;
        const uint32_t VAL = (BF_IDX < bitsField.size()) ? bitsField[BF_IDX] : 0;

        auto bitsStr= std::format("[{}:{}]", fm.endBit_, fm.startBit_);
        auto valStr = std::format("0x{:02X}", VAL);

        std::string detail;
        if(isRegField(fm.name_)) {
            detail= std::string(isa::LOOKUP_REG_NAME(VAL, false));
            detail+= " / ";
            detail+= std::string(isa::LOOKUP_REG_NAME(VAL, true));
        }

        std::format_to(out,
                       "  {}{}{}{}\n",
                       yellow(padRight(fm.name_, NAME_W)),
                       padRight(bitsStr, BITS_W),
                       green(padRight(valStr, VAL_W)),
                       detail);
    }

    std::print("{}", buf);
}

void OutputFormatter::printBitFieldDiagram(const Instruction &inst)
{
    const auto *pTypePtr= inst.GetTypePtr();
    if(!pTypePtr) return;

    const auto &bitsField= pTypePtr->GetInstBitsField();
    if(bitsField.empty()) return;

    InstFormat fmt = pTypePtr->GetInstFormat();
    const auto META= GetInstFields(fmt);
    if(META.empty()) return;

    const size_t TOTAL= META.size();

    std::array<size_t, 16> colWidths {};
    if(TOTAL > colWidths.size()) return;

    for(size_t i= 0; i < TOTAL; ++i) {
        int bitW    = META[i].endBit_ - META[i].startBit_ + 1;
        colWidths[i]= std::max(static_cast<size_t>(bitW), META[i].name_.size()) + 2;
    }

    std::string buf;
    buf.reserve(64 + TOTAL * 24);
    auto out= std::back_inserter(buf);

    auto spaces= [&](size_t n) {
        std::fill_n(out, n, ' ');
    };

    auto centered= [&](std::string_view s, size_t w) {
        size_t l= (w - s.size()) / 2;
        size_t r= w - s.size() - l;
        spaces(l);
        std::format_to(out, "{}", s);
        spaces(r);
    };

    auto colored= [&](std::string_view s, std::string_view code) {
        if(OutputMode::COLORED == Mode_) {
            std::format_to(out, "{}{}{}", code, s, G_C_RESET);
        } else {
            std::format_to(out, "{}", s);
        }
    };

    // std::println("  {}", bold(std::string(inst)));   // Line 1: Assembly

    // Line 2: Bit positions (endBit at left edge, startBit at right edge of each column)
    std::format_to(out, "\n  ");
    for(size_t i= 0; i < TOTAL; ++i) {
        std::format_to(out, " ");
        auto endStr  = std::to_string(META[i].endBit_);
        auto startStr= std::to_string(META[i].startBit_);
        size_t w     = colWidths[i];
        size_t mid   = w - endStr.size() - startStr.size();
        std::format_to(out, "{}", endStr);
        spaces(mid);
        std::format_to(out, "{}", startStr);
    }
    std::format_to(out, "\n  ");

    auto border= [&] {
        for(size_t i= 0; i < TOTAL; ++i) {
            std::format_to(out, "+");
            std::fill_n(out, colWidths[i], '-');
        }
        std::format_to(out, "+\n  ");
    };
    border(); // Line 3: Top border

    // Line 4: Binary values (centered within each column)
    for(size_t i= 0; i < TOTAL; ++i) {
        size_t bfIdx= TOTAL - 1 - i;
        uint32_t val= (bfIdx < bitsField.size()) ? bitsField[bfIdx] : 0;
        int bitW    = META[i].endBit_ - META[i].startBit_ + 1;

        std::string binStr;
        binStr.resize(bitW);
        for(int b= 0; b < bitW; ++b) {
            binStr[bitW - 1 - b]= (((val >> b) & 1U) != 0U) ? '1' : '0';
        }

        std::format_to(out, "|");
        size_t w= colWidths[i];
        size_t l= (w - binStr.size()) / 2;
        size_t r= w - binStr.size() - l;
        spaces(l);
        colored(binStr, G_C_GREEN);
        spaces(r);
    }
    std::format_to(out, "|\n  ");

    border(); // Line 5: Bottom border

    // Line 6: Field names (left-aligned, aligned to '-' after each '+')
    for(size_t i= 0; i < TOTAL; ++i) {
        std::format_to(out, " ");
        std::string_view name= META[i].name_;
        if(OutputMode::COLORED == Mode_) {
            std::format_to(out, "{}{}{}", G_C_YELLOW, name, G_C_RESET);
        } else {
            std::format_to(out, "{}", name);
        }
        if(name.size() < colWidths[i])
            spaces(colWidths[i] - name.size());
    }
    std::format_to(out, "\n");

    std::println("{}", buf);
}

void OutputFormatter::PrintRegister(uint16_t idx)
{
    if(OutputMode::JSON == Mode_) {
        printRegisterJSON(idx);
        return;
    }

    auto archName= isa::LOOKUP_REG_NAME(idx, false);
    auto abiName = isa::LOOKUP_REG_NAME(idx, true);

    constexpr size_t LABEL_W= 12;
    std::println("\n  {}\n  {}{}\n  {}{}\n  {}{}\n",
                 cyan("Register"),
                 bold(padRight("Index", LABEL_W)),
                 std::to_string(idx),
                 bold(padRight("Arch", LABEL_W)),
                 std::string(archName),
                 bold(padRight("ABI", LABEL_W)),
                 std::string(abiName));
}

void OutputFormatter::PrintInstructionList(const std::vector<InstrEntry> &entries)
{
    if(OutputMode::JSON == Mode_) {
        printListJSON(entries);
        return;
    }

    constexpr size_t NAME_W= 12, FMT_W= 10, XLEN_W= 12, OPC_W= 8;
    const size_t SEP_W= NAME_W + FMT_W + XLEN_W + OPC_W + 10 + 2;

    std::string buf;
    buf.reserve(96 + entries.size() * 48);
    auto out= std::back_inserter(buf);

    std::format_to(out, "\n  {}\n  {} entries\n\n", cyan("Supported instructions"), entries.size());
    std::format_to(out,
                   "  {}{}{}{}{}\n  {}\n",
                   bold(padRight("Name", NAME_W)),
                   bold(padRight("Format", FMT_W)),
                   bold(padRight("XLEN", XLEN_W)),
                   bold(padRight("Opcode", OPC_W)),
                   bold("FunctKey"),
                   std::string(SEP_W, '-'));

    for(const auto &e: entries) {
        std::format_to(out,
                       "  {}{}{}{}0x{:04X}\n",
                       yellow(padRight(std::string(e.name_), NAME_W)),
                       padRight(std::string(InstFormatName(e.format_)), FMT_W),
                       padRight(std::string(e.xlen_), XLEN_W),
                       green(padRight(std::format("0x{:02X}", e.opcode_), OPC_W)),
                       e.functKey_);
    }
    std::println("{}", buf);
}

void OutputFormatter::PrintError(std::string_view msg)
{
    if(OutputMode::JSON == Mode_) {
        std::println(R"({{"error":"{}"}})", jsonEscape(msg));
    } else {
        std::println(stderr, "{}{}", magenta("error: "), msg);
    }
}

void OutputFormatter::PrintHelp()
{
    std::println(
        "{} - RISC-V instruction encoder/decoder\n\n"
        "{}\n  rvinst <command> [options] [args]\n\n"
        "{}\n"
        "  {} <input>    Decode hex/binary to assembly\n"
        "  {} <input>    Encode assembly to hex/binary\n"
        "  {}            Interactive REPL mode\n"
        "  {} <query>    Look up register info\n"
        "  {} [--type T] List supported instructions\n\n"
        "{}\n"
        "  --abi             Use ABI register names (zero, ra, sp, ...)\n"
        "  --no-abi          Use architectural names (x0, x1, ...) [default]\n"
        "  --json            Output in JSON format\n"
        "  -h, --help        Show this help message\n"
        "  -v, --version     Show version information\n\n"
        "{}\n"
        "  Hex:      0x008100b3 or 008100b3\n"
        "  Binary:   0b00000000100000010000000010110011 or 32-bit 0/1 string\n"
        "  Assembly: add x1, x2, x8\n\n"
        "{}\n"
        "  rvinst decode 0x008100b3\n"
        "  rvinst encode \"add x1, x2, x8\"\n"
        "  rvinst decode 0x008100b3 --abi\n"
        "  rvinst reg a0\n"
        "  rvinst reg 10\n"
        "  rvinst list --type R\n"
        "  rvinst repl\n",
        bold("rvinst"),
        bold("Usage"),
        bold("Commands"),
        cyan("decode"),
        cyan("encode"),
        cyan("repl"),
        cyan("reg"),
        cyan("list"),
        bold("Global options"),
        bold("Input formats"),
        bold("Examples"));
}

std::string OutputFormatter::jsonEscape(std::string_view s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for(char c: s) {
        switch(c) {
        case '"':  out+= "\\\""; break;
        case '\\': out+= "\\\\"; break;
        case '\n': out+= "\\n"; break;
        case '\t': out+= "\\t"; break;
        case '\r': out+= "\\r"; break;
        default:   out+= c;
        }
    }
    return out;
}

void OutputFormatter::printInstructionJSON(const Instruction &inst, std::string_view input)
{
    const auto *pTp   = inst.GetTypePtr();
    const auto &bits  = pTp ? pTp->GetInstBitsField() : std::vector<uint32_t> {};
    InstFormat fmt    = pTp ? pTp->GetInstFormat() : InstFormat::UNKNOWN;
    const auto &meta  = GetInstFields(fmt);
    const size_t TOTAL= meta.size();

    std::string buf;
    buf.reserve(256 + TOTAL * 80);
    auto out= std::back_inserter(buf);

    std::format_to(out, "{{\n");
    std::format_to(out, R"(  "input": "{}",)"
                        "\n",
                   jsonEscape(input));
    std::format_to(out, R"(  "binary": "{}",)"
                        "\n",
                   inst.GetBinStr());
    std::format_to(out, R"(  "assembly": "{}",)"
                        "\n",
                   jsonEscape(std::string(inst)));
    std::format_to(out, R"(  "hex": "{}",)"
                        "\n",
                   inst.GetHexStr());
    std::format_to(out, R"(  "format": "{}",)"
                        "\n",
                   inst.GetFormat());
    std::format_to(out, R"(  "xlen": "{}",)"
                        "\n",
                   inst.GetXLEN());
    std::format_to(out, R"(  "manual": "{}",)"
                        "\n",
                   jsonEscape(std::string(inst.GetManual())));
    std::format_to(out, R"(  "fields": [)"
                        "\n");

    for(size_t i= 0; i < TOTAL; ++i) {
        const auto &fm     = meta[i];
        const size_t BF_IDX= TOTAL - 1 - i;
        const uint32_t VAL = (BF_IDX < bits.size()) ? bits[BF_IDX] : 0;

        std::format_to(out,
                       R"(    {{"name":"{}","bits":"[{}:{}]","value":"0x{:02X}")",
                       fm.name_,
                       fm.endBit_,
                       fm.startBit_,
                       VAL);
        if(isRegField(fm.name_)) {
            std::format_to(out, R"(,"register":"x{}")", VAL);
        }
        std::format_to(out, "}}");
        if(i + 1 < TOTAL) std::format_to(out, ",");

        std::format_to(out, "\n");
    }

    std::format_to(out, "  ]\n}}\n");
    std::print("{}", buf);
}

void OutputFormatter::printRegisterJSON(uint16_t idx)
{
    auto arch= isa::LOOKUP_REG_NAME(idx, false);
    auto abi = isa::LOOKUP_REG_NAME(idx, true);
    std::println(R"({{"index":{},"arch":"{}","abi":"{}"}})", idx, arch, abi);
}

void OutputFormatter::printListJSON(const std::vector<InstrEntry> &entries)
{
    std::string buf;
    buf.reserve(16 + entries.size() * 96);
    auto out= std::back_inserter(buf);
    std::format_to(out, "[\n");
    for(size_t i= 0; i < entries.size(); ++i) {
        const auto &e= entries[i];
        std::format_to(out,
                       R"(  {{"name":"{}","format":"{}","xlen":"{}","opcode":"0x{:02X}","functKey":"0x{:04X}"}})",
                       e.name_,
                       InstFormatName(e.format_),
                       e.xlen_,
                       e.opcode_,
                       e.functKey_);
        if(i + 1 < entries.size()) std::format_to(out, ",");
        std::format_to(out, "\n");
    }
    std::format_to(out, "]\n");
    std::print("{}", buf);
}

} // namespace cli
