#pragma once
#include <QListView>
#include <QWheelEvent>
#include "WLyricsModel.h"

class WLyricsPanel : public QListView
{
public:
    explicit WLyricsPanel();
    ~WLyricsPanel();

    void getCurrentRow(qint64 position_ms);
    void setRawLyrics(const QString& raw_data);
    void setLocalLrc(const QString& filepath);

private:
    WLyricsModel* m_lrcModel;
    void wheelEvent(QWheelEvent* event);
};