#pragma once

#include "core/dsl/evaluator.h"
#include "core/types.h"

#include <QString>

namespace dsl
{

/// TrackMetaData 的 Row 适配器(播放列表与媒体库通用)。
///
/// - filepathOverride: 当 meta.filepath 为空时使用(媒体库条目场景);
/// - missingOverride: >=0 时覆盖缺失判断(媒体库记录);
/// - set_index(): 提供 "index"(原始列表序号)上下文, 供播放列表排序/分组用。
class TrackMetaRow : public Row
{
public:
    TrackMetaRow(const TrackMetaData& meta, const QString& filepathOverride = {},
                 int missingOverride = -1);

    void set_index(int index)
    {
        index_ = index;
    }

    Value property(const QString& name) const override;

private:
    const TrackMetaData& meta_;
    QString filepath_;
    int missing_ = -1;
    int index_   = -1;
};

} // namespace dsl
