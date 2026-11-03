#include "w_lyrics_panel.h"

WLyricsPanel::WLyricsPanel(QWidget* parent) : QListView(parent)
{
    lrc_model_ = new WLyricsModel(this);
    this->setModel(lrc_model_);
    this->setWordWrap(true);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setFocusPolicy(Qt::NoFocus);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Prevent word-wrap-based minimum width from constraining the SidePanel.
    setMinimumWidth(0);
}

void WLyricsPanel::scroll_by_position(qint64 position_ms)
{
    int row = lrc_model_->get_row_by_position(position_ms);
    if (row >= 0 && row < lrc_model_->rowCount()) {
        QModelIndex idx = lrc_model_->index(row, 0);
        if (idx.isValid()) {
            this->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        }
    }
}

bool WLyricsPanel::set_raw_lyrics(const QString& raw_data)
{
    if (lrc_model_->set_raw_lyrics(raw_data)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::set_local_lrc(const QString& filepath)
{
    if (lrc_model_->set_local_lrc(filepath)) {
        return true;
    }
    return false;
}

bool WLyricsPanel::set_lrc_file_path(const QString& lrc_path)
{
    if (lrc_model_->set_lrc_file_path(lrc_path)) {
        return true;
    }
    return false;
}

QString WLyricsPanel::raw_lyrics_text() const
{
    return lrc_model_->raw_lyrics_text();
}

void WLyricsPanel::set_default_info(const TrackMetaData& meta)
{
    lrc_model_->set_default_info(meta);
}

void WLyricsPanel::wheelEvent(QWheelEvent* event)
{
    // do nothing to disable mouse wheel scroll
    event->accept();
}
