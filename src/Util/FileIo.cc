#include <format>
#include <ranges>
#include <system_error>
#include <vector>

#include "Util/FileIo.hh"

namespace util {

FileIo::FileIo(fs::path dataDir): DataDir_(std::move(dataDir)) { }

const fs::path &FileIo::GetDataDir() const noexcept { return DataDir_; }

fs::path FileIo::HistoryLogPath(std::string_view name) const
{
    return DataDir_ / name;
}

Expected_u<std::ifstream> FileIo::openInput(const fs::path &path)
{
    /*
     * std::in_place constructs the std::ifstream directly in the expected's
     * storage. A failed open leaves the stream in fail state but the expected
     * still counts as engaged, so check is_open() for the real status.
     */
    Expected_u<std::ifstream> in(std::in_place, path);
    if(!in->is_open()) {
        return Unexpected(
            Error::InvalidArg(std::format("cannot open file: {}", path.generic_string())));
    }
    return in;
}

Expected_u<void> FileIo::streamLines(const fs::path &path, LineCallback_u onLine)
{
    auto in= openInput(path);
    if(!in) return Unexpected(std::move(in.error()));

    std::string line;
    while(std::getline(*in, line)) {
        onLine(line);
    }
    return {};
}

Expected_u<void> FileIo::ForEachLine(std::string_view path, LineCallback_u onLine)
{
    return streamLines(fs::path(path), onLine); // Caller's path is used verbatim; relative paths resolve against the CWD.
}

Expected_u<void> FileIo::LoadHistory(InputHistoryMgr &hist) const
{
    std::error_code ec;
    const fs::path LOG= HistoryLogPath();
    if(!fs::exists(LOG, ec) || ec) {
        return {}; // no prior log: first run, nothing to load
    }

    // History is capped (e.g. 32 entries); materializing is cheap and lets us
    // Push in reverse so newest stays at the front after dedupe.
    std::vector<std::string> lines;
    auto streamed= streamLines(LOG, [&](std::string_view row) { lines.emplace_back(row); });
    if(!streamed) return Unexpected(std::move(streamed.error()));

    for(auto &it: std::views::reverse(lines)) {
        hist.Push(it);
    }
    return {};
}

Expected_u<void> FileIo::SaveHistory(const InputHistoryMgr &hist) const
{
    std::error_code ec;
    fs::create_directories(DataDir_, ec);
    if(ec) {
        return Unexpected(Error::InvalidArg(std::format("cannot create data dir: {}", ec.message())));
    }

    const fs::path LOG= HistoryLogPath();
    std::ofstream out(LOG, std::ios::trunc);
    if(!out) {
        return Unexpected(
            Error::InvalidArg(std::format("cannot open history log: {}", LOG.generic_string())));
    }

    for(const auto &entry: hist.GetEntries()) {
        out << entry << '\n';
    }
    return {};
}

Expected_u<void> FileIo::AppendHistory(std::string_view line) const
{
    std::error_code ec;
    fs::create_directories(DataDir_, ec);
    if(ec) {
        return Unexpected(Error::InvalidArg(std::format("cannot create data dir: {}", ec.message())));
    }

    const fs::path LOG= HistoryLogPath();
    std::ofstream out(LOG, std::ios::app);
    if(!out) {
        return Unexpected(
            Error::InvalidArg(std::format("cannot open history log: {}", LOG.generic_string())));
    }

    out << line << '\n';
    return {};
}

} // namespace util
