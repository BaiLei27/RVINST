#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <system_error>
#include <vector>

#include "Util/FileIo.hh"

namespace {

namespace fs= std::filesystem;

[[nodiscard]] fs::path tempFile(std::string_view name, std::string_view content)
{
    const auto P= fs::temp_directory_path() / std::string(name);
    std::ofstream out(P, std::ios::trunc);
    out << content;
    return P;
}

int failures= 0;

#define CHECK(cond, ...)                                                                 \
    do {                                                                                 \
        if(!(cond)) {                                                                    \
            ++failures;                                                                  \
            std::println("FAIL {}:{} {}", __FILE__, __LINE__, std::format(__VA_ARGS__)); \
        }                                                                                \
    } while(false)

} // namespace

int main()
{
    const auto DATA_ROOT= fs::temp_directory_path() / "rvinst_fileio_data";
    std::error_code ec;
    fs::create_directories(DATA_ROOT, ec);
    util::FileIo mgr(DATA_ROOT);

    // 1. ForEachLine on an existing file (blank lines preserved)
    const auto SRC= tempFile("rvinst_io_read.txt", "0x33\n\nadd x1, x2, x8\n");
    std::vector<std::string> lines;
    auto each= util::FileIo::ForEachLine(SRC.string(), [&](std::string_view row) {
        lines.emplace_back(row);
    });
    CHECK(bool(each), "ForEachLine should succeed");
    if(lines.size() == 3) {
        CHECK(lines[0] == "0x33", "line0 = {}", lines[0]);
        CHECK(lines[2] == "add x1, x2, x8", "line2 = {}", lines[2]);
    } else {
        CHECK(false, "expected 3 lines, got {}", lines.size());
    }

    // 2. ForEachLine on a missing file returns an Error
    const auto MISSING= (fs::temp_directory_path() / "rvinst_io_missing.txt").string();
    auto missing      = util::FileIo::ForEachLine(MISSING, [](std::string_view) { });
    CHECK(!missing, "ForEachLine of missing file should fail");

    // 3. DataDir is configurable and roots the history log only
    CHECK(mgr.GetDataDir() == DATA_ROOT, "injected data dir should stick");

    // 4. History round-trip: SaveHistory then LoadHistory
    util::InputHistoryMgr hist(32);
    hist.Push("add x1, x2, x8");
    hist.Push("0x33");
    CHECK(mgr.SaveHistory(hist), "SaveHistory should succeed");

    util::InputHistoryMgr loaded(32);
    CHECK(mgr.LoadHistory(loaded), "LoadHistory should succeed");
    CHECK(loaded.Size() == 2, "loaded size = {}", loaded.Size());
    if(loaded.Size() == 2) {
        CHECK(loaded.At(0) == "0x33", "most recent first, At(0) = {}", loaded.At(0));
        CHECK(loaded.At(1) == "add x1, x2, x8", "At(1) = {}", loaded.At(1));
    }

    // 5. LoadHistory on a fresh file (no prior log) still succeeds
    fs::remove(mgr.HistoryLogPath(), ec);
    util::InputHistoryMgr empty(32);
    CHECK(mgr.LoadHistory(empty), "LoadHistory with no log should succeed (no-op)");

    fs::remove_all(DATA_ROOT, ec);

    std::println("\nFileIo tests: {} failures", failures);
    return (failures == 0) ? 0 : 1;
}
