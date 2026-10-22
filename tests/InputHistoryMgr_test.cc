#include <format>
#include <print>
#include <string>

#include "Util/InputHistoryMgr.hh"

namespace {

int failures= 0;

#define CHECK(cond, ...)                                                             \
    do {                                                                             \
        if(!(cond)) {                                                                \
            ++failures;                                                              \
            std::println("FAIL {}:{} {}", __FILE__, __LINE__, std::format(__VA_ARGS__)); \
        }                                                                            \
    } while(false)

} // namespace

int main()
{
    util::InputHistoryMgr hist(3);

    hist.Push("a");
    hist.Push("b");
    hist.Push("a"); // dedupe + move to front
    CHECK(hist.Size() == 2, "size after dedupe = {}", hist.Size());
    CHECK(hist.At(0) == "a", "At(0) = {}", hist.At(0));
    CHECK(hist.At(1) == "b", "At(1) = {}", hist.At(1));

    hist.Push("c");
    hist.Push("d"); // cap at 3; drop oldest
    CHECK(hist.Size() == 3, "capped size = {}", hist.Size());
    CHECK(hist.At(0) == "d", "newest = {}", hist.At(0));
    CHECK(hist.At(2) == "a", "oldest kept = {}", hist.At(2));

    hist.SetDraft("draft");
    auto up= hist.Step(+1);
    CHECK(up && *up == "d", "first up should show newest");
    auto down= hist.Step(-1);
    CHECK(down && *down == "draft", "down from 0 restores draft");
    CHECK(!hist.IsBrowsing(), "draft restore clears browse cursor");

    CHECK(!hist.Step(-1), "down with no cursor is a no-op");

    std::println("\nInputHistoryMgr tests: {} failures", failures);
    return (failures == 0) ? 0 : 1;
}
