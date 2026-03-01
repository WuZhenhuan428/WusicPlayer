#pragma once

#include "model/player/player.h"
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

    void setPlayer(Player* player);
    void loadCover(const QString& filepath);
    void loadMetaData(const TrackMetaData& meta);
    WLyricsPanel* getLyricsPanel() const;
    bool loadLyrics(const TrackMetaData& meta);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel* m_coverLabel;
    WLyricsPanel* m_lyricsPanel;
    QPixmap m_originalCover;
    QSplitter* m_panelSplitter;

    ElidedLabel* m_nameLabel;
    ElidedLabel* m_albumLabel;

    QVBoxLayout* m_mainLayout;
    void updateCoverScale();
};