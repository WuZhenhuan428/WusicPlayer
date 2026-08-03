#pragma once
#include "w_lyrics_model.h"

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

    void scroll_by_position(qint64 position_ms);
    bool set_raw_lyrics(const QString& raw_data);
    bool set_local_lrc(const QString& filepath);
    bool set_lrc_file_path(const QString& lrc_path);
    QString raw_lyrics_text() const;
    void set_default_info(const TrackMetaData& meta);

private:
    WLyricsModel* m_lrc_model;
    void wheelEvent(QWheelEvent* event);
};
