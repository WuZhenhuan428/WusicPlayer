#pragma once

#include <QObject>
#include <QStack>

#include "../playlist_definitions.h"

namespace Playlist
{

class SortLevel
{
public:
    // 如何接入Qt::SortOrder?
    explicit SortLevel(SortType sort_type );
};

class SortSchema : public QObject
{
    Q_OBJECT;
public:
    explicit SortSchema(QObject* parent = nullptr);
    ~SortSchema();

public:
    void setSortSchema();
};


} // namespace Playlist