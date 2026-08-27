#pragma once

#include <QString>
#include <QStringList>

#include <expected>

namespace utils::string
{

QString unfold_string(const QStringList& list, char separator);

/// 不支持前导空格, 解析失败时返回失败位置
std::expected<int, size_t> string_to_int(const std::string& str) noexcept;

} // namespace utils::string
