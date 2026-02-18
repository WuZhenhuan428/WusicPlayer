#include "SidePanel.h"
#include "../../include/audio.h"
#include <QDebug>

#define DEFAULT_COVER_PATH "/home/wuzhenhuan/pictures/zhihu-meme.jpg"

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
{
    m_panelSplitter = new QSplitter(Qt::Vertical, this);
    
    m_coverLabel = new QLabel;
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_originalCover = new QPixmap;
    m_originalCover->load(DEFAULT_COVER_PATH);
    m_coverLabel->setPixmap(*m_originalCover);
    
    m_lyricsPanel = new WLyricsPanel;
    m_panelSplitter->addWidget(m_coverLabel);
    m_panelSplitter->addWidget(m_lyricsPanel);
    m_panelSplitter->setChildrenCollapsible(false);
    m_panelSplitter->setStretchFactor(0, 1);
    m_panelSplitter->setStretchFactor(1, 1);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addWidget(m_panelSplitter);

    this->setLayout(m_mainLayout);
}

SidePanel::~SidePanel() {}

void SidePanel::setPlayer(Player* player) {
    
}

void SidePanel::loadCover(const QString& filepath) {
    QPixmap pix = Audio::parse_cover_to_qpixmap(filepath.toStdString());
    if (!pix.isNull()) {
        *m_originalCover = pix;
    } else {
        m_originalCover->load(DEFAULT_COVER_PATH);
    }
    updateCoverScale();
}

void SidePanel::updateCoverScale() {
    if (!m_originalCover || m_originalCover->isNull()) {
        return;
    }
    int base_width = parentWidget()->geometry().width() / 4;;
    QPixmap scaled = m_originalCover->scaled(
        base_width,
        base_width,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    m_coverLabel->setFixedSize(base_width, base_width);
    m_coverLabel->setPixmap(scaled);
}

WLyricsPanel* SidePanel::getLyricsPanel() const {
    return m_lyricsPanel;
}

bool SidePanel::loadLyrics(const TrackMetaData& meta) {
    if (meta.isValid && !meta.lyrics.isEmpty()) {
        if (m_lyricsPanel->setRawLyrics(meta.lyrics)) {
            qDebug() << "[LRC] Loaded from metadata.";
        } else if (m_lyricsPanel->setLocalLrc(meta.filepath)) {
            qDebug() << "[LRC] Loaded from local .lrc file.";
        } else {
            m_lyricsPanel->setDefaultInfo(meta);
            qDebug() << "[LRC] Use default info";
        }
        return true;
    }
    return false;
}

void SidePanel::resizeEvent(QResizeEvent *event) {
    this->updateCoverScale();
    QWidget::resizeEvent(event);
}
