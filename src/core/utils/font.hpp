#pragma once

#include <QApplication>
#include <QFont>
#include <QString>

namespace utils::font
{
QFont get_relative_size_font_default(int relative_pt);
QFont get_system_mono_font();
} // namespace utils::font
