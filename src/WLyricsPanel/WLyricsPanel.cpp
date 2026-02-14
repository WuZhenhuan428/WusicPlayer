#include "WLyricsPanel.h"

WLyricsPanel::WLyricsPanel() {
    m_lrcModel = new WLyricsModel;
    this->setModel(m_lrcModel);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setFocusPolicy(Qt::NoFocus);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

WLyricsPanel::~WLyricsPanel() {}

void WLyricsPanel::getCurrentRow(qint64 position_ms) {
    int row = m_lrcModel->getCurrentRow(position_ms);
    if (row >= 0 && row < m_lrcModel->rowCount()) {
        QModelIndex idx = m_lrcModel->index(row, 0);
        if (idx.isValid()) {
            this->scrollTo(idx, QAbstractItemView::PositionAtCenter);
            // delegate
        }
    }
}

void WLyricsPanel::setRawLyrics(const QString& raw_data) {
    m_lrcModel->setRawLyrics(raw_data);
}

void WLyricsPanel::setLocalLrc(const QString& filepath) {
    m_lrcModel->setLocalLrc(filepath);
}

void WLyricsPanel::wheelEvent(QWheelEvent* event) {
    // do nothing to disable mouse wheel scroll
    event->accept();
}