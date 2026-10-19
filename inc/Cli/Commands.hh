#pragma once

#include <concepts>
#include <type_traits>

#include "Cli/OutputTable.hh"
#include "ISA/InstFormat.hh"
#include "Util/ErrorHandle.hh"
#include "Util/FileIo.hh"
#include "Util/InputHistoryMgr.hh"

namespace cli {

template <typename T>
concept InstInput= std::same_as<std::remove_cvref_t<T>, uint32_t> || std::same_as<std::remove_cvref_t<T>, std::string_view>;

struct GlobalOptions {
    bool hasABI_ { false };
    OutputMode outputMode_ { OutputMode::PLAIN };
};

class Commands {
private:
    GlobalOptions Opts_;
    OutputFormatter Fmt_;
    util::InputHistoryMgr History_ { 32 };
    util::FileIo FileMgr_;
    bool SaveHistory_ { false };

public:
    explicit Commands(const GlobalOptions &opts, bool saveHistory= false);

public:
    [[nodiscard]] util::Expected_u<void> Decode(std::string_view input);
    [[nodiscard]] util::Expected_u<void> Encode(std::string_view input);
    [[nodiscard]] util::Expected_u<void> ProcessFile(std::string_view path);
    int Repl();
    [[nodiscard]] util::Expected_u<void> Reg(std::string_view query);
    [[nodiscard]] util::Expected_u<void> List(std::optional<InstFormat> filter= std::nullopt);

private:
    [[nodiscard]] util::Expected_u<void> processInstruction(InstInput auto input);
    [[nodiscard]] util::Expected_u<void> processFileLines(std::string_view path);
    int replLoop();

    static void replBanner();
    static void replHelp();
    void replHistory();
    void replFile(std::string_view path);
};

} // namespace cli
