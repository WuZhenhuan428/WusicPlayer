#include "core/dsl/track_meta_row.h"

#include <QFileInfo>
#include <QStringList>

namespace dsl
{

namespace
{
/// utils::audio::format() 会把空字符串填充为 "Unknown xxx" 占位符。
/// DSL 语义: 这些占位符应视为空值(null), 使 nulls first/last 与 "unknown" 分组生效。
const QStringList kUnknownPlaceholders = {
    QStringLiteral("Unknown Album"),
    QStringLiteral("Unknown Artist"),
    QStringLiteral("Unknown Genre"),
};

Value string_prop(const QString& raw)
{
    if (raw.isEmpty() || kUnknownPlaceholders.contains(raw))
        return Value::null();
    return Value::from_string(raw);
}
} // namespace

TrackMetaRow::TrackMetaRow(const TrackMetaData& meta, const QString& filepathOverride,
                           int missingOverride) :
    meta_(meta), filepath_(filepathOverride), missing_(missingOverride)
{}

Value TrackMetaRow::property(const QString& name) const
{
    const QString fp = meta_.filepath.isEmpty() ? filepath_ : meta_.filepath;

    if (name == QStringLiteral("title"))
        return string_prop(meta_.title);
    if (name == QStringLiteral("artist"))
        return string_prop(meta_.artist);
    if (name == QStringLiteral("album"))
        return string_prop(meta_.album);
    if (name == QStringLiteral("album_artist"))
        return string_prop(meta_.album_artist);
    if (name == QStringLiteral("genre"))
        return string_prop(meta_.genre);
    if (name == QStringLiteral("composer"))
        return string_prop(meta_.composer);
    if (name == QStringLiteral("comment"))
        return string_prop(meta_.comment);
    if (name == QStringLiteral("lyrics"))
        return string_prop(meta_.lyrics);
    if (name == QStringLiteral("encoder"))
        return string_prop(meta_.encoder);
    if (name == QStringLiteral("date"))
        return string_prop(meta_.date);
    if (name == QStringLiteral("filename")) {
        const QString fn = meta_.filename.isEmpty() ? QFileInfo(fp).fileName() : meta_.filename;
        return string_prop(fn);
    }
    if (name == QStringLiteral("filepath"))
        return Value::from_string(fp);
    if (name == QStringLiteral("directory"))
        return Value::from_string(QFileInfo(fp).absolutePath());
    if (name == QStringLiteral("extension"))
        return Value::from_string(QFileInfo(fp).suffix());

    if (name == QStringLiteral("year"))
        return Value::from_int(meta_.year);
    if (name == QStringLiteral("track"))
        return Value::from_int(meta_.track_number);
    if (name == QStringLiteral("disc"))
        return Value::from_int(meta_.disc_number);
    if (name == QStringLiteral("disc_total"))
        return Value::from_int(meta_.disc_total);
    if (name == QStringLiteral("duration"))
        return Value::from_int(meta_.duration_s);
    if (name == QStringLiteral("bitrate"))
        return Value::from_int(meta_.bitrate);
    if (name == QStringLiteral("start_at"))
        return Value::from_int(meta_.start_at);
    if (name == QStringLiteral("index"))
        return index_ >= 0 ? Value::from_int(index_) : Value::null();

    if (name == QStringLiteral("missing")) {
        if (missing_ >= 0)
            return Value::from_bool(missing_ != 0);
        return fp.isEmpty() ? Value::null() : Value::from_bool(!QFileInfo(fp).exists());
    }

    return Value::null();
}

} // namespace dsl
