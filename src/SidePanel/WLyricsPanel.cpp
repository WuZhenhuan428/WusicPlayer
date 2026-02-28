#include "WLyricsPanel.h"
#include "../../src/core/types.h"

WLyricsPanel::WLyricsPanel(QWidget* parent)
    : QListView(parent)
{
    m_lrcModel = new WLyricsModel(this);
    this->setModel(m_lrcModel);
    this->setWordWrap(true);
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
        }
    }
}

bool WLyricsPanel::setRawLyrics(const QString& raw_data) {
    if (m_lrcModel->setRawLyrics(raw_data)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::setLocalLrc(const QString& filepath) {
    if (m_lrcModel->setLocalLrc(filepath)) {
        return true;
    }
    return false;
}

void WLyricsPanel::setDefaultInfo() {
    m_lrcModel->setDefaultInfo();
}

void WLyricsPanel::wheelEvent(QWheelEvent* event) {
    // do nothing to disable mouse wheel scroll
    event->accept();
}