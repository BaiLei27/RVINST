#pragma once

#include <filesystem>
#include <fstream>
#include <functional>

#include "Util/ErrorHandle.hh"
#include "Util/InputHistoryMgr.hh"
#include "config.hpp"

namespace util {

namespace fs= std::filesystem;
using namespace std::string_view_literals;

// std::function_ref (C++26, __cpp_lib_function_ref); fall back to std::function.
#if defined(__cpp_lib_function_ref) && __cpp_lib_function_ref >= 202306L
using LineCallback_u= std::function_ref<void(std::string_view)>;
#else
using LineCallback_u= std::function<void(std::string_view)>;
#endif

/**
 * @brief Bidirectional file IO shared by the CLI and GUI.
 * Batch inputs (ForEachLine) use the caller's path verbatim: the data
 * directory is never prefixed. DataDir_ roots only the history log.
 */
class FileIo {
private:
    fs::path DataDir_ { util::info::G_DATA_DIR }; // root for inputHist.log only
public:
    // Uses the build-time ${CMAKE_BINARY_DIR}/gen/data directory.
    FileIo()= default;
    // Inject a root for tests or alternate layouts.
    explicit FileIo(fs::path dataDir);

public:
    [[nodiscard]] const fs::path &GetDataDir() const noexcept;
    fs::path HistoryLogPath(std::string_view name= "inputHist.log"sv) const;

    // Stream each line; onLine must not retain the view past the call.
    static Expected_u<void> ForEachLine(std::string_view path, const LineCallback_u &onLine);

    Expected_u<void> LoadHistory(InputHistoryMgr &hist) const;
    Expected_u<void> SaveHistory(const InputHistoryMgr &hist) const;

private:
    static Expected_u<std::ifstream> openInput(const fs::path &path);
    static Expected_u<void> streamLines(const fs::path &path, const LineCallback_u &onLine);
};

} // namespace util
