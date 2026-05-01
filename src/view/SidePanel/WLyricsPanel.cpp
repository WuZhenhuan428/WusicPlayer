#include "WLyricsPanel.h"

WLyricsPanel::WLyricsPanel(QWidget* parent)
    : QListView(parent)
{
    m_lrc_model = new WLyricsModel(this);
    this->setModel(m_lrc_model);
    this->setWordWrap(true);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setFocusPolicy(Qt::NoFocus);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Prevent word-wrap-based minimum width from constraining the SidePanel.
    setMinimumWidth(0);
}

WLyricsPanel::~WLyricsPanel() {}

void WLyricsPanel::ScrollByPosition(qint64 position_ms) {
    int row = m_lrc_model->getRowByPosition(position_ms);
    if (row >= 0 && row < m_lrc_model->rowCount()) {
        QModelIndex idx = m_lrc_model->index(row, 0);
        if (idx.isValid()) {
            this->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        }
    }
}

bool WLyricsPanel::setRawLyrics(const QString& raw_data) {
    if (m_lrc_model->setRawLyrics(raw_data)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::setLocalLrc(const QString& filepath) {
    if (m_lrc_model->setLocalLrc(filepath)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::setLrcFilePath(const QString& lrc_path) {
    if (m_lrc_model->setLrcFilePath(lrc_path)) {
        return true;
    }
    return false;
}

QString WLyricsPanel::rawLyricsText() const {
    return m_lrc_model->rawLyricsText();
}

void WLyricsPanel::setDefaultInfo(const TrackMetaData& meta) {
    m_lrc_model->setDefaultInfo(meta);
}

void WLyricsPanel::wheelEvent(QWheelEvent* event) {
    // do nothing to disable mouse wheel scroll
    event->accept();
}