#pragma once
#include "WLyricsModel.h"

#include <QWidget>
#include <QListView>
#include <QWheelEvent>
#include <QObject>
#include "core/types.h"

class WLyricsPanel : public QListView
{
    Q_OBJECT
public:
    explicit WLyricsPanel(QWidget* parent = nullptr);
    ~WLyricsPanel();

    void ScrollByPosition(qint64 position_ms);
    bool setRawLyrics(const QString& raw_data);
    bool setLocalLrc(const QString& filepath);
    void setDefaultInfo(const TrackMetaData& meta);
private:
    WLyricsModel* m_lrc_model;
    void wheelEvent(QWheelEvent* event);
};