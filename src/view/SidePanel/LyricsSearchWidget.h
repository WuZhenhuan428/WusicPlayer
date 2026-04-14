#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QPointer>

#include "core/LyricsFetcher/lyrics_manager.h"

class LyricsSearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LyricsSearchWidget(QWidget* parent = nullptr);
    ~LyricsSearchWidget() = default;

    void setInitialQuery(const QString& title, const QString& artist);
    void setSearchContext(const lyrics_fetcher::TrackMeta& context);

signals:
    void sgnLyricSelected(const lyrics_fetcher::LyricMeta& meta);

private:
    void initUi();
    void initConnections();
    void updateResultTable(const QVector<lyrics_fetcher::LyricMeta>& results);

private:
    QLineEdit* m_le_query = nullptr;
    QPushButton* m_btn_search = nullptr;
    QTableView* m_tb_results = nullptr;
    QStandardItemModel* m_model = nullptr;

    QVector<lyrics_fetcher::LyricMeta> m_results;
    lyrics_fetcher::TrackMeta m_search_context;
    bool m_searching = false;
};
