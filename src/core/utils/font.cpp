#include "font.hpp"

#include <QFontDatabase>

namespace utils::font
{

QFont get_relative_size_font_default(int relative_pt)
{
    const QFont& default_font = QApplication::font();
    int curr_size             = default_font.pointSize();
    if (curr_size == -1) {
        curr_size = default_font.pixelSize();
    }
    if (curr_size == -1) {
        curr_size = 16;
    }
    QFont new_font = default_font;
    new_font.setPointSize(curr_size + relative_pt);
    return new_font;
}

QFont get_system_mono_font()
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(QApplication::font().pointSize());
    return mono;
}

} // namespace utils::font
