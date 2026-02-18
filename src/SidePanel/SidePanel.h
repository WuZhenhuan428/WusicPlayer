#pragma once

#include <QWidget>
#include <QString>
#include <QResizeEvent>
#include <QLabel>
#include <QPixmap>
#include <QSplitter>
#include <QVBoxLayout>

#include "../player/player.h"
#include "WLyricsPanel.h"
#include "../playlist/playlist_definitions.h"

class SidePanel : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanel(QWidget *parent = nullptr);
    ~SidePanel();

    void setPlayer(Player* player);
    void loadCover(const QString& filepath);
    WLyricsPanel* getLyricsPanel() const;
    bool loadLyrics(const TrackMetaData& meta);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel* m_coverLabel;
    WLyricsPanel* m_lyricsPanel;
    QPixmap* m_originalCover;
    QSplitter* m_panelSplitter;
    QVBoxLayout* m_mainLayout;
    void updateCoverScale();
};