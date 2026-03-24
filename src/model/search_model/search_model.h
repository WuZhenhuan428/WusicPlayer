#pragma once

#include <QObject>
#include <QAbstractTableModel>
#include "core/search_types.h"
#include <QString>
#include <QVector>

class ISearchBackend;

/*
一些简单的思路，也是问题：
1. 主列表建立完成之后会自动调用PlaylistViewModel::saveListToCache() ==> PlaylistRepo::writeJsonPlaylist()，
   因此~/.local/share/WusicPlayer/<playlist_id>.json的缓存文件一定存在。
2. 搜索策略：PlaylistViewModel::rebuildAsync() 的行为记忆不清楚，根据以下几种可能进行选择：
    1. 每次启动一定更新缓存：
        可以在建立主视图完成后发送信号，通知搜索面板允许开始搜索，在此之前先等待（时间不会太久）
    2. 检测到有本地缓存则直接加载：
        获取上一次的playlist_id，如果存在对应的缓存则直接开始搜索，但是发送对应的视图信号应当在软件内缓存，
        等待主视图加载完成后发送，以保证视图和播放内容的正常
3. 搜索实现：
    根据2.2，直接获取本地缓存/PlaylistRepo并遍历，有相关元素则加入class内的QVector<TrackMetaData>，并加入视图，
    根据行查找class内的QVector，向PlaylistManager发送trackId即可实现播放。
    // ViewModel中使用TrackMetaData以保证搜索面板显示结果的可定制&多样化
    // SearchHind只包含实际搜索用得到的项，用于与搜索后端对接，数据类型与struct TrackMetaData对应项保持兼容
    // SearchHint中添加Score（评分，1-5星），目前可以空置，未来插入数据库，作为本地信息保存。
4. 资源管理：
    整个搜索面板使用懒加载+退出销毁？这个体量的软件对资源和性能并不是特别敏感，方便开发即可
*/


class SearchModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class Column : int {
        Title = 0,
        Artist,
        AlbumArtist,
        Album,
        Duration,
        Score,
        Count
    };
    
    explicit SearchModel(ISearchBackend* backend = nullptr, QObject* parent = nullptr);
    ~SearchModel();

    void searchRequest(SearchQuery query);
    void setResults(const QVector<SearchHint>& results);
    void setBackend(ISearchBackend* backend);
    void clearResults();

    trackId trackIdAt(int row) const;
    SearchHint hintAt(int row) const;
    int totalHits() const;
    const SearchQuery& lastQuery() const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    ISearchBackend* m_backend = nullptr;
    QVector<SearchHint> m_search_hint;
    SearchQuery m_last_query;
};