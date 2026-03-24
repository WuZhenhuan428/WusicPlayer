#pragma once

#include "core/search_types.h"

class ISearchBackend
{
public:
    virtual ~ISearchBackend() = default;

    virtual void warmup(const playlistId& pid) = 0;
    virtual void invalidate(const playlistId& pid) = 0;
    virtual QVector<SearchHint> search(const SearchQuery& query) = 0;
};