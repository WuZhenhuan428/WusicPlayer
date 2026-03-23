#include "core/utils/AudioUtils.h"
#include "SidePanel.h"
#include <QDebug>
#include <QTimer>

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

    m_lyrics_panel = new WLyricsPanel(this);
    m_panel_splitter->addWidget(m_lb_cover);
    m_panel_splitter->addWidget(m_lb_title);
    m_panel_splitter->addWidget(m_lb_album);
    m_panel_splitter->addWidget(m_lyrics_panel);
    m_panel_splitter->setChildrenCollapsible(false);
    m_panel_splitter->setStretchFactor(0, 1);
    m_panel_splitter->setStretchFactor(1, 0);
    m_panel_splitter->setStretchFactor(2, 0);
    m_panel_splitter->setStretchFactor(3, 1);

    m_vbl_main = new QVBoxLayout(this);
    m_vbl_main->addWidget(m_panel_splitter);

    m_panel_splitter->setStyleSheet("QSplitter::handle { background: transparent; }");

    this->setLayout(m_vbl_main);
}

SidePanel::~SidePanel() { }

void SidePanel::loadCover(const QString& filepath) {
    QPixmap pix = AudioUtils::parse_cover_to_qpixmap(filepath.toStdString());
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
    int base_width = this->parentWidget()->width() / 4.5;
    QPixmap scaled = m_original_cover.scaled(
        base_width,
        base_width,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_lb_cover->setMinimumSize(0, 0);
    m_lb_cover->setMaximumSize(base_width, base_width);
    m_lb_cover->setPixmap(scaled);
}

WLyricsPanel* SidePanel::getLyricsPanel() const {
    return m_lyrics_panel;
}

bool SidePanel::loadLyrics(const TrackMetaData& meta) {
    if (meta.isValid) {
        if (!meta.lyrics.isEmpty()) {
            m_lyrics_panel->setRawLyrics(meta.lyrics);
            qDebug() << "[LRC] Loaded from metadata.";
        } else if (m_lyrics_panel->setLocalLrc(meta.filepath)) {
            qDebug() << "[LRC] Loaded from local .lrc file.";
        } else {
            m_lyrics_panel->setDefaultInfo();
            qDebug() << "[LRC] Use default info";
        }
        return true;
    }
    return false;
}

void SidePanel::resizeEvent(QResizeEvent *event) {
    QTimer::singleShot(1, [this](){
        updateCoverScale();
    });
    for (int i = 0; i < m_panel_splitter->count(); ++i) {
        QSplitterHandle* handle = m_panel_splitter->handle(i);
        if (handle) {
            handle->setEnabled(false);
        }
    }
    QWidget::resizeEvent(event);
}
