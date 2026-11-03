#pragma once

#include "core/types.h"
#include "view/side_panel/elided_label.h"
#include "view/side_panel/w_lyrics_panel.h"

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

    void load_cover(const QString& filepath);
    void load_meta_data(const TrackMetaData& meta);
    [[nodiscard]] WLyricsPanel* get_lyrics_panel() const;
    bool load_lyrics(const TrackMetaData& meta);

    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void sgn_desktop_lyrics_config_requested();
    void sgn_toggle_desktop_lyrics();
    void sgn_toggle_desktop_lyricsLock();

private slots:
    void show_lyrics_context_menu(const QPoint& pos);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* lb_cover_;
    WLyricsPanel* lyrics_panel_;
    QPixmap original_cover_;
    QSplitter* panel_splitter_;

    ElidedLabel* lb_title_;
    ElidedLabel* lb_album_;

    QVBoxLayout* vbl_main_;
    QTimer* resize_timer_;
    TrackMetaData last_lyrics_meta_;
    bool has_last_lyrics_meta_ = false;
    QPointer<LyricsSearchWidget> lyrics_search_widget_;
    void update_cover_scale();
};
