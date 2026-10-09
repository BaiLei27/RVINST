#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <array>
#endif

namespace tst {

struct InstCase {
    std::string kind_;
    std::string input_;
    std::string expectHex_;
    std::string expectAsm_;
    std::string expectFormat_;
    std::string expectManualTail_;
    std::string status_ { "impl" }; // impl | todo
};

[[nodiscard]] std::string toUpper(std::string s)
{
    for(char &c: s) {
        if(c >= 'a' && c <= 'z') c= static_cast<char>(c - 'a' + 'A');
    }
    return s;
}

[[nodiscard]] std::string jsonField(std::string_view json, std::string_view key)
{
    const auto NEEDLE= std::format("\"{}\"", key);
    auto pos         = json.find(NEEDLE);
    if(std::string_view::npos == pos) return {};

    pos= json.find(':', pos + NEEDLE.size());
    if(std::string_view::npos == pos) return {};
    pos= json.find('"', pos + 1);
    if(std::string_view::npos == pos) return {};
    ++pos;
    const auto END= json.find('"', pos);
    if(std::string_view::npos == END) return {};
    return std::string(json.substr(pos, END - pos));
}

[[nodiscard]] std::string quoteArg(std::string_view s)
{
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for(char c: s) {
        if('"' == c) {
            out+= "\\\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

#ifdef _WIN32

[[nodiscard]] std::string runProcess(const std::filesystem::path &exe,
                                     std::string_view kind,
                                     std::string_view input)
{
    SECURITY_ATTRIBUTES sa {};
    sa.nLength       = sizeof(sa);
    sa.bInheritHandle= TRUE;

    HANDLE pHRead {}, pHWrite {};
    if(0 == CreatePipe(&pHRead, &pHWrite, &sa, 0)) return {};
    SetHandleInformation(pHRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si {};
    si.cb        = sizeof(si);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdOutput= pHWrite;
    si.hStdError = pHWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    const auto EXE_W= exe.wstring();
    std::wstring cmd=
        L"\"" + EXE_W + L"\" --json " + std::wstring(kind.begin(), kind.end()) + L" "
        + L"\"" + std::wstring(input.begin(), input.end()) + L"\"";

    PROCESS_INFORMATION pi {};
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    const BOOL OK= CreateProcessW(EXE_W.c_str(),
                                  mutableCmd.data(),
                                  nullptr,
                                  nullptr,
                                  TRUE,
                                  CREATE_NO_WINDOW,
                                  nullptr,
                                  nullptr,
                                  &si,
                                  &pi);
    CloseHandle(pHWrite);
    if(0 == OK) {
        CloseHandle(pHRead);
        return {};
    }

    std::string out;
    char buf[512];
    DWORD n= 0;
    while((ReadFile(pHRead, buf, sizeof(buf), &n, nullptr) != 0) && n > 0) {
        out.append(buf, buf + n);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pHRead);
    return out;
}

#else

[[nodiscard]] std::string runProcess(const std::filesystem::path &exe,
                                     std::string_view kind,
                                     std::string_view input)
{
    const auto CMD=
        std::format("{} --json {} {}", quoteArg(exe.string()), kind, quoteArg(input));
    FILE *pPipe= popen(CMD.c_str(), "r");
    if(!pPipe) return {};

    std::string out;
    std::array<char, 512> buf {};
    while(std::fgets(buf.data(), static_cast<int>(buf.size()), pPipe) != nullptr) {
        out+= buf.data();
    }
    pclose(pPipe);
    return out;
}

#endif

} // namespace tst

/** Drive RVINST_cli against vector cases and report pass/fail. */
class InstTester {
public:
    InstTester(std::filesystem::path cliPath, std::filesystem::path casesPath)
        : CliPath_(std::move(cliPath)),
          CasesPath_(std::move(casesPath)) { }

    [[nodiscard]] int Run()
    {
        if(!std::filesystem::exists(CliPath_)) {
            std::println(stderr, "CLI not found: {}", CliPath_.string());
            return 2;
        }
        if(!loadCases()) return 2;

        std::println("\nRVINST CLI instruction tests");
        std::println("CLI  : {}", CliPath_.string());
        std::println("Cases: {}", CasesPath_.string());
        std::println("{:-<72}", "");

        for(const auto &c: Cases_) {
            runOne(c);
        }

        std::println("{:-<72}", "");
        std::println("Result: {} passed, {} todo, {} failed, {} total",
                     Passed_,
                     Todo_,
                     Failed_,
                     Cases_.size());
        if(Failed_ > 0) {
            std::println(stderr, "Failed cases: {}", FailedLabels_);
            return 1;
        }
        if(Todo_ > 0) {
            std::println("Note: {} case(s) still TODO (not implemented / incorrect).", Todo_);
        }
        std::println("All required (impl) CLI instruction tests passed.");
        return 0;
    }

private:
    std::filesystem::path CliPath_;
    std::filesystem::path CasesPath_;
    std::vector<tst::InstCase> Cases_;
    size_t Passed_ {};
    size_t Todo_ {};
    size_t Failed_ {};
    std::string FailedLabels_;

    [[nodiscard]] bool loadCases()
    {
        std::ifstream in(CasesPath_);
        if(!in) {
            std::println(stderr, "Cannot open cases file: {}", CasesPath_.string());
            return false;
        }

        std::string line;
        while(std::getline(in, line)) {
            while(!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            if(line.empty() || line.starts_with('#')) continue;

            tst::InstCase c;
            std::istringstream ss(line);
            if(!std::getline(ss, c.kind_, '|') || !std::getline(ss, c.input_, '|')
               || !std::getline(ss, c.expectHex_, '|') || !std::getline(ss, c.expectAsm_, '|')
               || !std::getline(ss, c.expectFormat_, '|')
               || !std::getline(ss, c.expectManualTail_, '|')) {
                std::println(stderr, "SKIP  bad line: {}", line);
                continue;
            }
            if(!std::getline(ss, c.status_) || c.status_.empty()) c.status_= "impl";
            c.expectHex_= tst::toUpper(std::move(c.expectHex_));
            Cases_.emplace_back(std::move(c));
        }

        if(Cases_.empty()) {
            std::println(stderr, "No cases loaded from {}", CasesPath_.string());
            return false;
        }
        return true;
    }

    void runOne(const tst::InstCase &c)
    {
        const bool IS_TODO= (c.status_ == "todo");
        const auto LABEL  = std::format("{} {}", c.kind_, c.input_);
        const auto CLI_OUT= tst::runProcess(CliPath_, c.kind_, c.input_);

        const auto HEX= tst::toUpper(tst::jsonField(CLI_OUT, "hex"));
        const auto ASM= tst::jsonField(CLI_OUT, "assembly");
        const auto FMT= tst::jsonField(CLI_OUT, "format");
        const auto MAN= tst::jsonField(CLI_OUT, "manual");

        std::vector<std::string> errs;
        if(CLI_OUT.empty()) errs.emplace_back("empty CLI output");
        if(c.expectHex_ != HEX) {
            errs.emplace_back(std::format("hex got={} want={}", HEX, c.expectHex_));
        }
        if(c.expectAsm_ != ASM) {
            errs.emplace_back(std::format("asm got='{}' want='{}'", ASM, c.expectAsm_));
        }
        if(c.expectFormat_ != FMT) {
            errs.emplace_back(std::format("format got={} want={}", FMT, c.expectFormat_));
        }
        const auto TAIL= std::format("/{}", c.expectManualTail_);
        if(MAN.empty() || !MAN.ends_with(TAIL)) {
            errs.emplace_back(
                std::format("manual tail got='{}' want='...{}'", MAN, TAIL));
        }

        if(errs.empty()) {
            ++Passed_;
            std::println("PASS  {}", LABEL);
            return;
        }

        if(IS_TODO) {
            ++Todo_;
            std::println("TODO  {}  ({})", LABEL, errs.front());
            return;
        }

        ++Failed_;
        if(!FailedLabels_.empty()) FailedLabels_+= "; ";
        FailedLabels_+= LABEL;
        std::println("FAIL  {}", LABEL);
        for(const auto &e: errs) {
            std::println("        - {}", e);
        }
    }
};

int main(int argc, char *pArgv[])
{
    if(argc < 3) {
        std::println(stderr, "Usage: {} <RVINST_cli> <inst_sets.txt>", pArgv[0]);
        return 2;
    }

    InstTester tester(pArgv[1], pArgv[2]);
    return tester.Run();
}
