#include "core/utils/AudioUtils.h"
#include "SidePanel.h"
#include "LyricsSearchWidget.h"
#include <QDebug>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>

#define DEFAULT_COVER_PATH ":/images/test_cover_chirno.png"

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
{
    m_panel_splitter = new QSplitter(Qt::Vertical, this);
    
    m_lb_cover = new QLabel(this);
    m_lb_cover->setAlignment(Qt::AlignCenter);
    m_original_cover.load(DEFAULT_COVER_PATH);
    m_lb_cover->setPixmap(m_original_cover);

    m_lb_title = new ElidedLabel(this);
    m_lb_title->setText("WusicPlayer");    // format: "title - artist"
    m_lb_title->setAlignment(Qt::AlignHCenter);

    m_lb_album = new ElidedLabel(this);
    m_lb_album->setText("Version 0.01");
    m_lb_album->setAlignment(Qt::AlignHCenter);

    m_lb_title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_lb_album->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_lb_title->setFixedHeight(m_lb_title->sizeHint().height());
    m_lb_album->setFixedHeight(m_lb_album->sizeHint().height());

    m_lyrics_panel = new WLyricsPanel(this);
        m_lyrics_panel->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_lyrics_panel, &QWidget::customContextMenuRequested,
            this, &SidePanel::showLyricsContextMenu);
    m_panel_splitter->addWidget(m_lb_cover);
    m_panel_splitter->addWidget(m_lb_title);
    m_panel_splitter->addWidget(m_lb_album);
    m_panel_splitter->addWidget(m_lyrics_panel);
    m_panel_splitter->setChildrenCollapsible(false);

    m_vbl_main = new QVBoxLayout(this);
    m_vbl_main->addWidget(m_panel_splitter);

    // m_panel_splitter->setStyleSheet("QSplitter::handle { background: transparent; }");

    this->setLayout(m_vbl_main);

    m_panel_splitter->setContentsMargins(0, 0, 0, 0);
    m_vbl_main->setContentsMargins(0, 0, 0, 0);
    this->setContentsMargins(0, 0, 0, 0);

    // Debounce timer: avoids calling updateCoverScale() on every
    // intermediate resize event during window drag.  The cover is
    // updated only after the user pauses (or the drag ends).
    m_resize_timer = new QTimer(this);
    m_resize_timer->setSingleShot(true);
    m_resize_timer->setInterval(10);
    connect(m_resize_timer, &QTimer::timeout, this, &SidePanel::updateCoverScale);
}

SidePanel::~SidePanel() { }

void SidePanel::loadCover(const QString& filepath) {
    QPixmap pix = AudioUtils::parse_cover_to_qpixmap(filepath);
    if (!pix.isNull()) {
        m_original_cover = pix;
    } else {
        m_original_cover.load(DEFAULT_COVER_PATH);
    }
    updateCoverScale();
}

void SidePanel::loadMetaData(const TrackMetaData& meta) {
    if (!meta.isValid) {
        return;
    }

    if (meta.artist == "Unknown Artist") {
        m_lb_title->setFullText(meta.title);
    } else {
        QString name = meta.title + " - " + meta.artist;
        m_lb_title->setFullText(name);
    }
    m_lb_album->setFullText(meta.album);
}

void SidePanel::updateCoverScale() {
    if (!m_original_cover || m_original_cover.isNull()) {
        return;
    }
    // Use SidePanel's own width instead of parent width to avoid
    // locking the minimum width during asynchronous resize updates.
    int base_width = this->width();
    if (base_width <= 0) {
        return;
    }
    QPixmap scaled = m_original_cover.scaled(
        base_width,
        base_width,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_lb_cover->setFixedHeight(base_width);
    m_lb_cover->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_lb_cover->setPixmap(scaled);
}

QSize SidePanel::minimumSizeHint() const {
    // The cover QLabel's pixmap-based minimumSizeHint() propagates up
    // through QSplitter -> QVBoxLayout -> SidePanel, blocking the
    // MainWindow QHBoxLayout from shrinking this panel.  We pin the
    // horizontal minimum to 0 so the stretch-based layout can always
    // contract freely.
    return QSize(0, QWidget::minimumSizeHint().height());
}

WLyricsPanel* SidePanel::getLyricsPanel() const {
    return m_lyrics_panel;
}

bool SidePanel::loadLyrics(const TrackMetaData& meta) {
    m_last_lyrics_meta = meta;
    m_has_last_lyrics_meta = true;

    if (meta.isValid) {
        if (!meta.lyrics.isEmpty()) {
            m_lyrics_panel->setRawLyrics(meta.lyrics);
            qDebug() << "[LRC] Loaded from metadata.";
        } else if (m_lyrics_panel->setLocalLrc(meta.filepath)) {
            qDebug() << "[LRC] Loaded from local .lrc file.";
        } else {
            m_lyrics_panel->setDefaultInfo(meta);
            qDebug() << "[LRC] Use default info";
        }
        return true;
    }
    return false;
}

void SidePanel::resizeEvent(QResizeEvent *event) {
    // Restart the debounce timer on every resize so that the cover
    // pixmap is only re-scaled once the user stops dragging.
    m_resize_timer->start();

    for (int i = 0; i < m_panel_splitter->count(); ++i) {
        QSplitterHandle* handle = m_panel_splitter->handle(i);
        if (handle) {
            handle->setEnabled(false);
        }
    }
    QWidget::resizeEvent(event);
}

void SidePanel::showLyricsContextMenu(const QPoint& pos) {
    QMenu menu(this);
    QAction* actDesktopConfig = menu.addAction(tr("Desktop Lyrics Config"));
    QAction* actLoadLyrics = menu.addAction(tr("Load Lyrics"));
    QAction* actReloadLyrics = menu.addAction(tr("Reload Lyrics"));
    QAction* actSearchLyrics = menu.addAction(tr("Lyrics Search"));
    QMenu* subSave = menu.addMenu(tr("Save Lyrics As"));
    QAction* actSaveToTag = subSave->addAction(tr("Save To Tag"));
    QAction* actSaveToSource = subSave->addAction(tr("Save To Source Location"));
    QAction* actSaveAs = subSave->addAction(tr("Save As"));

    auto getCurrentLyricsText = [this]() -> QString {
        return m_lyrics_panel ? m_lyrics_panel->rawLyricsText().trimmed() : QString();
    };

    auto ensureLyricsExists = [this, &getCurrentLyricsText]() -> bool {
        if (getCurrentLyricsText().isEmpty()) {
            QMessageBox::information(this,
                                     tr("No Lyrics"),
                                     tr("Current playing audio has no lyrics."));
            return false;
        }
        return true;
    };

    auto ensureLrcSuffix = [](QString path) -> QString {
        if (!path.endsWith(".lrc", Qt::CaseInsensitive)) {
            path += ".lrc";
        }
        return path;
    };

    auto writeLyricsFile = [this](const QString& path, const QString& text) {
        QFile out(path);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Unable to write lyrics file."));
            return;
        }
        out.write(text.toUtf8());
        out.close();
    };

    connect(actDesktopConfig, &QAction::triggered, this, [this]() {
        emit sgnDesktopLyricsConfigRequested();
    });

    connect(actLoadLyrics, &QAction::triggered, this, [this]() {
        const QString file_path = QFileDialog::getOpenFileName(
            this,
            tr("Load Lyrics File"),
            QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)")
        );

        if (file_path.isEmpty()) {
            return;
        }

        if (!m_lyrics_panel->setLrcFilePath(file_path)) {
            QMessageBox::warning(this,
                                 tr("Lyrics Format Error"),
                                 tr("The selected file format is invalid."));
        }
    });

    connect(actReloadLyrics, &QAction::triggered, this, [this]() {
        if (!m_has_last_lyrics_meta) {
            return;
        }
        loadLyrics(m_last_lyrics_meta);
    });

    connect(actSaveToTag, &QAction::triggered, this, [this, &getCurrentLyricsText, &ensureLyricsExists]() {
        if (!ensureLyricsExists()) {
            return;
        }
        if (!m_has_last_lyrics_meta || m_last_lyrics_meta.filepath.isEmpty()) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Unable to locate current audio file."));
            return;
        }

        QMap<QString, QStringList> tags;
        tags.insert("LYRICS", QStringList{getCurrentLyricsText()});
        const bool ok = AudioUtils::taglib_writeback(m_last_lyrics_meta.filepath, tags);
        if (!ok) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Unable to save lyrics into tags."));
        }
    });

    connect(actSaveToSource, &QAction::triggered, this,
            [this, &getCurrentLyricsText, &ensureLyricsExists, &ensureLrcSuffix, &writeLyricsFile]() {
        if (!ensureLyricsExists()) {
            return;
        }
        if (!m_has_last_lyrics_meta || m_last_lyrics_meta.filepath.isEmpty()) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Unable to locate current audio file."));
            return;
        }

        QFileInfo audio_info(m_last_lyrics_meta.filepath);
        const QString target = ensureLrcSuffix(
            audio_info.absolutePath() + "/" + audio_info.completeBaseName());
        writeLyricsFile(target, getCurrentLyricsText());
    });

    connect(actSaveAs, &QAction::triggered, this,
            [this, &getCurrentLyricsText, &ensureLyricsExists, &ensureLrcSuffix, &writeLyricsFile]() {
        if (!ensureLyricsExists()) {
            return;
        }

        QString file_path = QFileDialog::getSaveFileName(
            this,
            tr("Save Lyrics As"),
            QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)")
        );
        if (file_path.isEmpty()) {
            return;
        }

        file_path = ensureLrcSuffix(file_path);
        writeLyricsFile(file_path, getCurrentLyricsText());
    });

    connect(actSearchLyrics, &QAction::triggered, this, [this]() {
        if (!m_lyrics_search_widget) {
            m_lyrics_search_widget = new LyricsSearchWidget(this);
            m_lyrics_search_widget->setWindowFlag(Qt::Window, true);
            m_lyrics_search_widget->setAttribute(Qt::WA_DeleteOnClose, true);

            connect(m_lyrics_search_widget, &LyricsSearchWidget::sgnLyricSelected,
                    this, [this](const lyrics_fetcher::LyricMeta& meta) {
                if (meta.lyricText.trimmed().isEmpty()) {
                    QMessageBox::information(this, tr("Lyrics Search"), tr("Selected result has empty lyrics."));
                    return;
                }
                m_lyrics_panel->setRawLyrics(meta.lyricText);
            });

            connect(m_lyrics_search_widget, &QObject::destroyed, this, [this]() {
                m_lyrics_search_widget = nullptr;
            });
        }

        if (m_has_last_lyrics_meta) {
            m_lyrics_search_widget->setInitialQuery(m_last_lyrics_meta.title, m_last_lyrics_meta.artist);

            lyrics_fetcher::TrackMeta context;
            context.rawTitle = m_last_lyrics_meta.title;
            context.rawArtist = m_last_lyrics_meta.artist;
            context.rawAlbum = m_last_lyrics_meta.album;
            context.durationSec = m_last_lyrics_meta.duration_s;
            m_lyrics_search_widget->setSearchContext(context);
        }

        m_lyrics_search_widget->show();
        m_lyrics_search_widget->raise();
        m_lyrics_search_widget->activateWindow();
    });

    menu.exec(m_lyrics_panel->viewport()->mapToGlobal(pos));
}
