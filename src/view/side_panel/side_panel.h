#pragma once

#include "view/side_panel/elided_label.h"
#include "view/side_panel/w_lyrics_panel.h"
#include "core/types.h"

#include <QLabel>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QResizeEvent>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class LyricsSearchWidget;

class SidePanel : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanel(QWidget* parent = nullptr);
    ~SidePanel();

    void load_cover(const QString& filepath);
    void load_meta_data(const TrackMetaData& meta);
    WLyricsPanel* get_lyrics_panel() const;
    bool load_lyrics(const TrackMetaData& meta);

    QSize minimumSizeHint() const override;

signals:
    void sgnDesktopLyricsConfigRequested();
    void sgnToggleDesktopLyrics();
    void sgnToggleDesktopLyricsLock();

private slots:
    void show_lyrics_context_menu(const QPoint& pos);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* m_lb_cover;
    WLyricsPanel* m_lyrics_panel;
    QPixmap m_original_cover;
    QSplitter* m_panel_splitter;

    ElidedLabel* m_lb_title;
    ElidedLabel* m_lb_album;

    QVBoxLayout* m_vbl_main;
    QTimer* m_resize_timer;
    TrackMetaData m_last_lyrics_meta;
    bool m_has_last_lyrics_meta = false;
    QPointer<LyricsSearchWidget> m_lyrics_search_widget;
    void update_cover_scale();
};
