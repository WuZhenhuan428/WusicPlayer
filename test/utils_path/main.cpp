#include "core/utils/path.hpp"

#include <QString>
#include <QStringList>

static int g_CHECKs;
static int g_failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        ++g_CHECKs;                                                                                \
        if (!(cond)) {                                                                             \
            ++g_failures;                                                                          \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                            \
        }                                                                                          \
    } while (0)

using namespace utils::path;

int main()
{
    QString root    = "/abs/path/";

    QStringList sos = {
        "bar.so.11",
        "baz.so.11.22",
        "foobar.so.11.22.33",
    };

    QString normal        = "foo.so";

    QStringList err_paths = {
        "is_path.so.6.9/",
        "have_no_dot_so",
        "suffixes_too_long.so.11.22.33.44",
        "have_empty_fileld_1.so..2.3",
        "have_empty_fileld_2.so.1..3",
        "have_empty_fileld_3.so.1.2.",
    };

    bool OK = false;
    OK      = contains_suffix(normal, {"so"});
    CHECK(OK); // pass
    // normal
    OK = !parse_versioned_unix_so(normal).has_value();
    CHECK(OK);

    // relative:
    for (const auto& path : sos) {
        OK = parse_versioned_unix_so(path).has_value();
        CHECK(OK);
    }

    // absolute
    for (const auto& path : sos) {
        OK = parse_versioned_unix_so(root + path).has_value();
        CHECK(OK);
    }

    // error state
    for (const auto& err : err_paths) {
        OK = !parse_versioned_unix_so(err).has_value();
        CHECK(OK);
    }

    std::printf("== utils_path: %d CHECKs, %d failures ==\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
