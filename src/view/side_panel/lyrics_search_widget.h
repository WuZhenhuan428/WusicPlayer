#pragma once

#include "core/lyrics_fetcher/lyrics_manager.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class LyricsSearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LyricsSearchWidget(QWidget* parent = nullptr);

    void set_initial_query(const QString& title, const QString& artist);
    void set_search_context(const lyrics_fetcher::TrackMeta& context);

signals:
    void sgn_lyrics_selected(const lyrics_fetcher::LyricMeta& meta);

private:
    void init_ui();
    void init_connections();
    void update_result_table(const QVector<lyrics_fetcher::LyricMeta>& results);

private:
    QLineEdit* le_query_       = nullptr;
    QPushButton* btn_search_   = nullptr;
    QTableView* tb_results_    = nullptr;
    QStandardItemModel* model_ = nullptr;

    QVector<lyrics_fetcher::LyricMeta> results_;
    lyrics_fetcher::TrackMeta search_context_;
    bool searching_ = false;
};
