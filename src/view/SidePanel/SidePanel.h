#pragma once

#include "WLyricsPanel.h"
#include "core/types.h"
#include "ElidedLabel.h"

#include <QWidget>
#include <QString>
#include <QResizeEvent>
#include <QLabel>
#include <QPixmap>
#include <QSplitter>
#include <QVBoxLayout>


class SidePanel : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanel(QWidget *parent = nullptr);
    ~SidePanel();

    void loadCover(const QString& filepath);
    void loadMetaData(const TrackMetaData& meta);
    WLyricsPanel* getLyricsPanel() const;
    bool loadLyrics(const TrackMetaData& meta);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel* m_lb_cover;
    WLyricsPanel* m_lyrics_panel;
    QPixmap m_original_cover;
    QSplitter* m_panel_splitter;

    ElidedLabel* m_lb_title;
    ElidedLabel* m_lb_album;

    QVBoxLayout* m_vbl_main;
    void updateCoverScale();
};