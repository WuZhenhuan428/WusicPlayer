#include "w_lyrics_panel.h"

WLyricsPanel::WLyricsPanel(QWidget* parent) : QListView(parent)
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

void WLyricsPanel::scroll_by_position(qint64 position_ms)
{
    int row = m_lrc_model->get_row_by_position(position_ms);
    if (row >= 0 && row < m_lrc_model->rowCount()) {
        QModelIndex idx = m_lrc_model->index(row, 0);
        if (idx.isValid()) {
            this->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        }
    }
}

bool WLyricsPanel::set_raw_lyrics(const QString& raw_data)
{
    if (m_lrc_model->set_raw_lyrics(raw_data)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::set_local_lrc(const QString& filepath)
{
    if (m_lrc_model->set_local_lrc(filepath)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::set_lrc_file_path(const QString& lrc_path)
{
    if (m_lrc_model->set_lrc_file_path(lrc_path)) {
        return true;
    }
    return false;
}

QString WLyricsPanel::raw_lyrics_text() const
{
    return m_lrc_model->raw_lyrics_text();
}

void WLyricsPanel::set_default_info(const TrackMetaData& meta)
{
    m_lrc_model->set_default_info(meta);
}

void WLyricsPanel::wheelEvent(QWheelEvent* event)
{
    // do nothing to disable mouse wheel scroll
    event->accept();
}
