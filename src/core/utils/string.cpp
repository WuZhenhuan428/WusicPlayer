#include "string.hpp"

#include <charconv>
#include <system_error>

namespace utils::string
{

QString unfold_string(const QStringList& list, char separator)
{
    if (list.size() == 0) {
        return {};
    }
    QString str = list[0];
    for (ssize_t i = 1; i < list.size(); ++i) {
        str += separator;
        str += list[i];
    }
    return str;
}

std::expected<int, size_t> string_to_int(const std::string& str) noexcept
{
    if (str.empty()) {
        return std::unexpected<size_t>(0);
    }

    int value         = 0;
    const char* first = str.data();
    const char* last  = first + str.size();

    auto [ptr, ec]    = std::from_chars(first, last, value);

    if (ec == std::errc()) {
        return value;
    }

    size_t failed_pos = 0;
    if (ec == std::errc::invalid_argument) {
        if (ptr == first) {
            failed_pos = 0;
        } else {
            failed_pos = static_cast<size_t>(ptr - first);
        }
    } else if (ec == std::errc::result_out_of_range) {
        if (ptr > first) { // overflow
            failed_pos = static_cast<size_t>(ptr - first);
        } else {
            failed_pos = 0;
        }
    }
    if (failed_pos > str.size()) {
        failed_pos = str.size();
    }
    return std::unexpected<size_t>(failed_pos);
}
} // namespace utils::string
