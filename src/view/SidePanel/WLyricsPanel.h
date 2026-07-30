#pragma once
#include "WLyricsModel.h"

#include "core/types.h"
#include <QListView>
#include <QObject>
#include <QWheelEvent>
#include <QWidget>

class WLyricsPanel : public QListView
{
    Q_OBJECT
public:
    explicit WLyricsPanel(QWidget* parent = nullptr);
    ~WLyricsPanel();

    void ScrollByPosition(qint64 position_ms);
    bool setRawLyrics(const QString& raw_data);
    bool setLocalLrc(const QString& filepath);
    bool setLrcFilePath(const QString& lrc_path);
    QString rawLyricsText() const;
    void setDefaultInfo(const TrackMetaData& meta);

private:
    WLyricsModel* m_lrc_model;
    void wheelEvent(QWheelEvent* event);
};
