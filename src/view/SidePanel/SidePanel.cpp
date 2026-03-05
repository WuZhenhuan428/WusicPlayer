#include "core/utils/AudioUtils.h"
#include "SidePanel.h"
#include <QDebug>

#define DEFAULT_COVER_PATH ":/images/test_cover_chirno.png"

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
{
    m_panelSplitter = new QSplitter(Qt::Vertical, this);
    
    m_coverLabel = new QLabel(this);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_originalCover.load(DEFAULT_COVER_PATH);
    m_coverLabel->setPixmap(m_originalCover);

    m_nameLabel = new ElidedLabel(this);
    m_nameLabel->setText("WusicPlayer");    // format: "title - artist"
    m_nameLabel->setAlignment(Qt::AlignHCenter);

    m_albumLabel = new ElidedLabel(this);
    m_albumLabel->setText("Version 0.01");
    m_albumLabel->setAlignment(Qt::AlignHCenter);

    m_lyricsPanel = new WLyricsPanel(this);
    m_panelSplitter->addWidget(m_coverLabel);
    m_panelSplitter->addWidget(m_nameLabel);
    m_panelSplitter->addWidget(m_albumLabel);
    m_panelSplitter->addWidget(m_lyricsPanel);
    m_panelSplitter->setChildrenCollapsible(false);
    m_panelSplitter->setStretchFactor(0, 1);
    m_panelSplitter->setStretchFactor(1, 0);
    m_panelSplitter->setStretchFactor(2, 0);
    m_panelSplitter->setStretchFactor(3, 1);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->addWidget(m_panelSplitter);

    m_panelSplitter->setStyleSheet("QSplitter::handle { background: transparent; }");

    this->setLayout(m_mainLayout);
}

SidePanel::~SidePanel() { }

void SidePanel::setPlayer(Player* player) {
    
}

void SidePanel::loadCover(const QString& filepath) {
    QPixmap pix = AudioUtils::parse_cover_to_qpixmap(filepath.toStdString());
    if (!pix.isNull()) {
        m_originalCover = pix;
    } else {
        m_originalCover.load(DEFAULT_COVER_PATH);
    }
    updateCoverScale();
}

void SidePanel::loadMetaData(const TrackMetaData& meta) {
    if (!meta.isValid) {
        return;
    }

    if (meta.artist == "Unknown Artist") {
        m_nameLabel->setFullText(meta.title);
    } else {
        QString name = meta.title + " - " + meta.artist;
        m_nameLabel->setFullText(name);
    }
    m_albumLabel->setFullText(meta.album);
}

void SidePanel::updateCoverScale() {
    if (!m_originalCover || m_originalCover.isNull()) {
        return;
    }
    int base_width = this->parentWidget()->width() / 4.5;
    QPixmap scaled = m_originalCover.scaled(
        base_width,
        base_width,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_coverLabel->setMinimumSize(0, 0);
    m_coverLabel->setMaximumSize(base_width, base_width);
    m_coverLabel->setPixmap(scaled);
}

WLyricsPanel* SidePanel::getLyricsPanel() const {
    return m_lyricsPanel;
}

bool SidePanel::loadLyrics(const TrackMetaData& meta) {
    if (meta.isValid) {
        if (!meta.lyrics.isEmpty()) {
            m_lyricsPanel->setRawLyrics(meta.lyrics);
            qDebug() << "[LRC] Loaded from metadata.";
        } else if (m_lyricsPanel->setLocalLrc(meta.filepath)) {
            qDebug() << "[LRC] Loaded from local .lrc file.";
        } else {
            m_lyricsPanel->setDefaultInfo();
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
    for (int i = 0; i < m_panelSplitter->count(); ++i) {
        QSplitterHandle* handle = m_panelSplitter->handle(i);
        if (handle) {
            handle->setEnabled(false);
        }
    }
    QWidget::resizeEvent(event);
}
