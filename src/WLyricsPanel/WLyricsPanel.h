#pragma once
#include <QListView>
#include <QWheelEvent>
#include "WLyricsModel.h"
#include "../playlist/playlist_definitions.h"

class WLyricsPanel : public QListView
{
public:
    explicit WLyricsPanel();
    ~WLyricsPanel();

    void getCurrentRow(qint64 position_ms);
    bool setRawLyrics(const QString& raw_data);
    bool setLocalLrc(const QString& filepath);
    void setDefaultInfo(const QString& filename, const QString& artist);
private:
    WLyricsModel* m_lrcModel;
    void wheelEvent(QWheelEvent* event);
};