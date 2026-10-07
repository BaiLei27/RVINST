#pragma once

#include <cstdint>
#include <iosfwd>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

#include "Cli/InstrEntry.hh"
#include "Core/Instruction.hh"

namespace cli {

/**
 * @brief
 * RAII guard: temporarily redirects std::cout to /dev/null during Parse() so the backend's debug prints don't pollute CLI output.
 */
class CoutSuppressor {
    std::streambuf *OldBuf_ { nullptr };

public:
    CoutSuppressor();
    ~CoutSuppressor();
};

enum class OutputMode : uint8_t {
    COLORED,
    PLAIN,
    JSON
};

class OutputFormatter {
private:
    OutputMode Mode_;

public:
    explicit OutputFormatter(OutputMode mode);

public:
    void PrintInstruction(const Instruction &inst, std::string_view input);
    void PrintRegister(uint16_t idx);
    void PrintInstructionList(const std::vector<InstrEntry> &entries);
    void PrintError(std::string_view msg);
    void PrintHelp();

    [[nodiscard]] OutputMode Mode() const noexcept;

private:
    [[nodiscard]] std::string paint(std::string_view text, std::string_view code) const;
    [[nodiscard]] std::string bold(std::string_view text) const;
    [[nodiscard]] std::string cyan(std::string_view text) const;
    [[nodiscard]] std::string green(std::string_view text) const;
    [[nodiscard]] std::string yellow(std::string_view text) const;
    [[nodiscard]] std::string magenta(std::string_view text) const;

    void printFieldTable(const Instruction &inst);
    void printBitFieldDiagram(const Instruction &inst);

    static void printInstructionJSON(const Instruction &inst, std::string_view input);
    static void printRegisterJSON(uint16_t idx);
    static void printListJSON(const std::vector<InstrEntry> &entries);
    static std::string jsonEscape(std::string_view s);
};

OutputMode detectOutputMode(bool forceJson);

} // namespace cli
