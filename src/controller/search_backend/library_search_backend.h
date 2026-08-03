#pragma once

#include "controller/search_backend/i_search_backend.h"

#include <QVector>

class LibraryManager;

/**
 * @brief 基于 SQLite FTS5 的搜索后端:直接搜索音乐库。
 *
 * 库索引由 LibraryManager 持续维护,无需 warmup/invalidate(实现为空)。
 * 返回结果的 `SearchHint::track_id` 为库级身份,播放由上层解析为文件路径。
 */
class LibrarySearchBackend : public ISearchBackend
{
public:
    explicit LibrarySearchBackend(LibraryManager* library);
    ~LibrarySearchBackend() override = default;

    void warmup(const PlaylistId& pid) override;
    void invalidate(const PlaylistId& pid) override;
    QVector<SearchHint> search(const SearchQuery& query) override;

private:
    LibraryManager* library_ = nullptr; // 非拥有
};
