#pragma once
#include "WLyricsModel.h"
#include "core/types.h"

#include <QWidget>
#include <QListView>
#include <QWheelEvent>
#include <QObject>

class WLyricsPanel : public QListView
{
    Q_OBJECT
public:
    explicit WLyricsPanel(QWidget* parent = nullptr);
    ~WLyricsPanel();

    void ScrollByPosition(qint64 position_ms);
    bool setRawLyrics(const QString& raw_data);
    bool setLocalLrc(const QString& filepath);
    void setDefaultInfo();
private:
    WLyricsModel* m_lrcModel;
    void wheelEvent(QWheelEvent* event);
};