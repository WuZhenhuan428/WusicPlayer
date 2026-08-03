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
    ~LyricsSearchWidget() = default;

    void set_initial_query(const QString& title, const QString& artist);
    void set_search_context(const lyrics_fetcher::TrackMeta& context);

signals:
    void sgnLyricSelected(const lyrics_fetcher::LyricMeta& meta);

private:
    void init_ui();
    void init_connections();
    void update_result_table(const QVector<lyrics_fetcher::LyricMeta>& results);

private:
    QLineEdit* m_le_query       = nullptr;
    QPushButton* m_btn_search   = nullptr;
    QTableView* m_tb_results    = nullptr;
    QStandardItemModel* m_model = nullptr;

    QVector<lyrics_fetcher::LyricMeta> m_results;
    lyrics_fetcher::TrackMeta m_search_context;
    bool m_searching = false;
};
