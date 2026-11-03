#include "view/side_panel/side_panel.h"
#include "core/utils/audio.hpp"
#include "lyrics_search_widget.h"

#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

const QString gDefaultCoverPath = ":/images/test_cover_chirno.png";

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("side_panel", {"console", "gui"});
} // namespace

SidePanel::SidePanel(QWidget* parent) : QWidget(parent)
{
    panel_splitter_ = new QSplitter(Qt::Vertical, this);

    lb_cover_       = new QLabel(this);
    lb_cover_->setAlignment(Qt::AlignCenter);
    original_cover_.load(gDefaultCoverPath);
    lb_cover_->setPixmap(original_cover_);

    lb_title_ = new ElidedLabel(this);
    lb_title_->setText("WusicPlayer"); // format: "title - artist"
    lb_title_->setAlignment(Qt::AlignHCenter);

    lb_album_ = new ElidedLabel(this);
    lb_album_->setText(tr("Version 0.01"));
    lb_album_->setAlignment(Qt::AlignHCenter);

    lb_title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    lb_album_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    lb_title_->setFixedHeight(lb_title_->sizeHint().height());
    lb_album_->setFixedHeight(lb_album_->sizeHint().height());

    lyrics_panel_ = new WLyricsPanel(this);
    lyrics_panel_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(lyrics_panel_, &QWidget::customContextMenuRequested, this,
            &SidePanel::show_lyrics_context_menu);
    panel_splitter_->addWidget(lb_cover_);
    panel_splitter_->addWidget(lb_title_);
    panel_splitter_->addWidget(lb_album_);
    panel_splitter_->addWidget(lyrics_panel_);
    panel_splitter_->setChildrenCollapsible(false);

    vbl_main_ = new QVBoxLayout(this);
    vbl_main_->addWidget(panel_splitter_);

    // panel_splitter_->setStyleSheet("QSplitter::handle { background: transparent; }");

    this->setLayout(vbl_main_);

    panel_splitter_->setContentsMargins(0, 0, 0, 0);
    vbl_main_->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0, 0, 0, 0);

    // Debounce timer: avoids calling update_cover_scale() on every
    // intermediate resize event during window drag.  The cover is
    // updated only after the user pauses (or the drag ends).
    resize_timer_ = new QTimer(this);
    resize_timer_->setSingleShot(true);
    resize_timer_->setInterval(10);
    connect(resize_timer_, &QTimer::timeout, this, &SidePanel::update_cover_scale);
}

void SidePanel::load_cover(const QString& filepath)
{
    QPixmap pix = utils::audio::parse_cover_to_qpixmap(filepath);
    if (!pix.isNull()) {
        original_cover_ = std::move(pix);
    } else {
        original_cover_.load(gDefaultCoverPath);
    }
    update_cover_scale();
}

void SidePanel::load_meta_data(const TrackMetaData& meta)
{
    if (!meta.is_valid) {
        return;
    }

    if (meta.artist == "Unknown Artist") {
        lb_title_->set_full_text(meta.title);
    } else {
        QString name = meta.title + " - " + meta.artist;
        lb_title_->set_full_text(name);
    }
    lb_album_->set_full_text(meta.album);
}

void SidePanel::update_cover_scale()
{
    if (!original_cover_ || original_cover_.isNull()) {
        return;
    }
    // Use SidePanel's own width instead of parent width to avoid
    // locking the minimum width during asynchronous resize updates.
    int base_width = this->width();
    if (base_width <= 0) {
        return;
    }
    QPixmap scaled = original_cover_.scaled(base_width, base_width, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
    lb_cover_->setFixedHeight(base_width);
    lb_cover_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    lb_cover_->setPixmap(scaled);
}

QSize SidePanel::minimumSizeHint() const
{
    // The cover QLabel's pixmap-based minimumSizeHint() propagates up
    // through QSplitter -> QVBoxLayout -> SidePanel, blocking the
    // MainWindow QHBoxLayout from shrinking this panel.  We pin the
    // horizontal minimum to 0 so the stretch-based layout can always
    // contract freely.
    return {0, QWidget::minimumSizeHint().height()};
}

WLyricsPanel* SidePanel::get_lyrics_panel() const
{
    return lyrics_panel_;
}

bool SidePanel::load_lyrics(const TrackMetaData& meta)
{
    last_lyrics_meta_     = meta;
    has_last_lyrics_meta_ = true;

    if (meta.is_valid) {
        if (!meta.lyrics.isEmpty()) {
            lyrics_panel_->set_raw_lyrics(meta.lyrics);
            logger->debug("[LRC] Loaded from metadata.");
        } else if (lyrics_panel_->set_local_lrc(meta.filepath)) {
            logger->debug("[LRC] Loaded from local .lrc file.");
        } else {
            lyrics_panel_->set_default_info(meta);
            logger->debug("[LRC] Use default info");
        }
        return true;
    }
    return false;
}

void SidePanel::resizeEvent(QResizeEvent* event)
{
    // Restart the debounce timer on every resize so that the cover
    // pixmap is only re-scaled once the user stops dragging.
    resize_timer_->start();

    for (int i = 0; i < panel_splitter_->count(); ++i) {
        QSplitterHandle* handle = panel_splitter_->handle(i);
        if (handle) {
            handle->setEnabled(false);
        }
    }
    QWidget::resizeEvent(event);
}

void SidePanel::show_lyrics_context_menu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* act_load_lyrics    = menu.addAction(tr("Load Lyrics"));
    QAction* act_reload_lyrics  = menu.addAction(tr("Reload Lyrics"));
    QAction* act_search_lyrics  = menu.addAction(tr("Lyrics Search"));
    QMenu* sub_save             = menu.addMenu(tr("Save Lyrics As"));
    QAction* act_save_to_tag    = sub_save->addAction(tr("Save To Tag"));
    QAction* act_save_to_source = sub_save->addAction(tr("Save To Source Location"));
    QAction* act_save_as        = sub_save->addAction(tr("Save As"));

    menu.addSeparator();
    QAction* act_desktop_config  = menu.addAction(tr("Desktop Lyrics Config"));
    QAction* act_toggle_lyrics   = menu.addAction(tr("Show/Hide Desktop Lyrics"));
    QAction* act_toggle_lock     = menu.addAction(tr("Lock/Unlock Desktop Lyrics"));

    auto get_current_lyrics_text = [this]() -> QString {
        return lyrics_panel_ ? lyrics_panel_->raw_lyrics_text().trimmed() : QString();
    };

    auto ensure_lyrics_exists = [this, &get_current_lyrics_text]() -> bool {
        if (get_current_lyrics_text().isEmpty()) {
            QMessageBox::information(this, tr("No Lyrics"),
                                     tr("Current playing audio has no lyrics."));
            return false;
        }
        return true;
    };

    auto ensure_lrc_suffix = [](QString path) -> QString {
        if (!path.endsWith(".lrc", Qt::CaseInsensitive)) {
            path += ".lrc";
        }
        return path;
    };

    auto write_lyrics_file = [this](const QString& path, const QString& text) {
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Unable to write lyrics file."));
            return;
        }
        out.write(text.toUtf8());
        out.close();
    };

    connect(act_desktop_config, &QAction::triggered, this,
            [this]() { emit sgn_desktop_lyrics_config_requested(); });

    connect(act_toggle_lyrics, &QAction::triggered, this,
            [this]() { emit sgn_toggle_desktop_lyrics(); });
    connect(act_toggle_lock, &QAction::triggered, this,
            [this]() { emit sgn_toggle_desktop_lyricsLock(); });

    connect(act_load_lyrics, &QAction::triggered, this, [this]() {
        const QString file_path = QFileDialog::getOpenFileName(
            this, tr("Load Lyrics File"), QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));

        if (file_path.isEmpty()) {
            return;
        }

        if (!lyrics_panel_->set_lrc_file_path(file_path)) {
            QMessageBox::warning(this, tr("Lyrics Format Error"),
                                 tr("The selected file format is invalid."));
        }
    });

    connect(act_reload_lyrics, &QAction::triggered, this, [this]() {
        if (!has_last_lyrics_meta_) {
            return;
        }
        load_lyrics(last_lyrics_meta_);
    });

    connect(act_save_to_tag, &QAction::triggered, this,
            [this, &get_current_lyrics_text, &ensure_lyrics_exists]() {
                if (!ensure_lyrics_exists()) {
                    return;
                }
                if (!has_last_lyrics_meta_ || last_lyrics_meta_.filepath.isEmpty()) {
                    QMessageBox::warning(this, tr("Save Failed"),
                                         tr("Unable to locate current audio file."));
                    return;
                }

                QMap<QString, QStringList> tags;
                tags.insert("LYRICS", QStringList{get_current_lyrics_text()});
                const bool ok = utils::audio::taglib_writeback(last_lyrics_meta_.filepath, tags);
                if (!ok) {
                    QMessageBox::warning(this, tr("Save Failed"),
                                         tr("Unable to save lyrics into tags."));
                }
            });

    connect(act_save_to_source, &QAction::triggered, this,
            [this, &get_current_lyrics_text, &ensure_lyrics_exists, &ensure_lrc_suffix,
             &write_lyrics_file]() {
                if (!ensure_lyrics_exists()) {
                    return;
                }
                if (!has_last_lyrics_meta_ || last_lyrics_meta_.filepath.isEmpty()) {
                    QMessageBox::warning(this, tr("Save Failed"),
                                         tr("Unable to locate current audio file."));
                    return;
                }

                QFileInfo audio_info(last_lyrics_meta_.filepath);
                const QString target = ensure_lrc_suffix(audio_info.absolutePath() + "/" +
                                                         audio_info.completeBaseName());
                write_lyrics_file(target, get_current_lyrics_text());
            });

    connect(act_save_as, &QAction::triggered, this,
            [this, &get_current_lyrics_text, &ensure_lyrics_exists, &ensure_lrc_suffix,
             &write_lyrics_file]() {
                if (!ensure_lyrics_exists()) {
                    return;
                }

                QString file_path = QFileDialog::getSaveFileName(
                    this, tr("Save Lyrics As"), QString(),
                    tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));
                if (file_path.isEmpty()) {
                    return;
                }

                file_path = ensure_lrc_suffix(file_path);
                write_lyrics_file(file_path, get_current_lyrics_text());
            });

    connect(act_search_lyrics, &QAction::triggered, this, [this]() {
        if (!lyrics_search_widget_) {
            lyrics_search_widget_ = new LyricsSearchWidget(this);
            lyrics_search_widget_->setWindowFlag(Qt::Window, true);
            lyrics_search_widget_->setAttribute(Qt::WA_DeleteOnClose, true);

            connect(lyrics_search_widget_, &LyricsSearchWidget::sgn_lyrics_selected, this,
                    [this](const lyrics_fetcher::LyricMeta& meta) {
                        if (meta.lyric_text.trimmed().isEmpty()) {
                            QMessageBox::information(this, tr("Lyrics Search"),
                                                     tr("Selected result has empty lyrics."));
                            return;
                        }
                        lyrics_panel_->set_raw_lyrics(meta.lyric_text);
                    });

            connect(lyrics_search_widget_, &QObject::destroyed, this,
                    [this]() { lyrics_search_widget_ = nullptr; });
        }

        if (has_last_lyrics_meta_) {
            lyrics_search_widget_->set_initial_query(last_lyrics_meta_.title,
                                                     last_lyrics_meta_.artist);

            lyrics_fetcher::TrackMeta context;
            context.raw_title    = last_lyrics_meta_.title;
            context.raw_artist   = last_lyrics_meta_.artist;
            context.raw_album    = last_lyrics_meta_.album;
            context.duration_sec = last_lyrics_meta_.duration_s;
            lyrics_search_widget_->set_search_context(context);
        }

        lyrics_search_widget_->show();
        lyrics_search_widget_->raise();
        lyrics_search_widget_->activateWindow();
    });

    menu.exec(lyrics_panel_->viewport()->mapToGlobal(pos));
}
