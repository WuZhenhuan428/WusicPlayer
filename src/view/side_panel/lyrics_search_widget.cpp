#include "lyrics_search_widget.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QThread>

namespace
{
lyrics_fetcher::TrackMeta parse_query_to_meta(const QString& query)
{
    lyrics_fetcher::TrackMeta meta;
    const QString trimmed   = query.trimmed();
    const QStringList parts = trimmed.split(" - ", Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        meta.raw_title  = parts.at(0).trimmed();
        meta.raw_artist = parts.at(1).trimmed();
    } else {
        meta.raw_title = trimmed;
        meta.raw_artist.clear();
    }
    return meta;
}
} // namespace

LyricsSearchWidget::LyricsSearchWidget(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void LyricsSearchWidget::set_initial_query(const QString& title, const QString& artist)
{
    const QString query = artist.trimmed().isEmpty()
                              ? title.trimmed()
                              : QString("%1 - %2").arg(title.trimmed(), artist.trimmed());
    le_query_->setText(query.trimmed());
}

void LyricsSearchWidget::set_search_context(const lyrics_fetcher::TrackMeta& context)
{
    search_context_ = context;
}

void LyricsSearchWidget::init_ui()
{
    le_query_ = new QLineEdit(this);
    le_query_->setPlaceholderText(tr("Input title or title - artist"));

    btn_search_ = new QPushButton(tr("Search"), this);

    tb_results_ = new QTableView(this);
    model_      = new QStandardItemModel(this);
    model_->setColumnCount(4);
    model_->setHeaderData(0, Qt::Horizontal, tr("Title"));
    model_->setHeaderData(1, Qt::Horizontal, tr("Artist"));
    model_->setHeaderData(2, Qt::Horizontal, tr("Album"));
    model_->setHeaderData(3, Qt::Horizontal, tr("Source"));

    tb_results_->setModel(model_);
    tb_results_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tb_results_->setSelectionMode(QAbstractItemView::SingleSelection);
    tb_results_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tb_results_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tb_results_->horizontalHeader()->setStretchLastSection(true);
    tb_results_->verticalHeader()->setVisible(false);

    auto* hbl_top = new QHBoxLayout();
    hbl_top->addWidget(le_query_);
    hbl_top->addWidget(btn_search_);

    auto* vbl_main = new QVBoxLayout();
    vbl_main->addLayout(hbl_top);
    vbl_main->addWidget(tb_results_);

    setLayout(vbl_main);
    setMinimumSize(640, 420);
    setWindowTitle(tr("Lyrics Search"));
}

void LyricsSearchWidget::init_connections()
{
    connect(btn_search_, &QPushButton::clicked, this, [this]() {
        if (searching_) {
            return;
        }

        const QString query = le_query_->text().trimmed();
        if (query.isEmpty()) {
            return;
        }

        searching_ = true;
        btn_search_->setEnabled(false);
        model_->removeRows(0, model_->rowCount());
        results_.clear();

        lyrics_fetcher::TrackMeta track_meta = parse_query_to_meta(query);
        if (track_meta.raw_artist.trimmed().isEmpty()) {
            track_meta.raw_artist = search_context_.raw_artist;
        }
        if (track_meta.raw_album.trimmed().isEmpty()) {
            track_meta.raw_album = search_context_.raw_album;
        }
        if (track_meta.duration_sec <= 0) {
            track_meta.duration_sec = search_context_.duration_sec;
        }
        QPointer<LyricsSearchWidget> self(this);

        QThread* worker = QThread::create([self, track_meta]() {
            QVector<lyrics_fetcher::LyricMeta> results;
            QNetworkAccessManager nam;

            lyrics_fetcher::LyricsManager manager;
            results = manager.fetch(track_meta, &nam);

            QMetaObject::invokeMethod(
                self,
                [self, results]() {
                    if (!self) {
                        return;
                    }
                    self->update_result_table(results);
                    self->searching_ = false;
                    self->btn_search_->setEnabled(true);
                },
                Qt::QueuedConnection);
        });

        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
    });

    connect(le_query_, &QLineEdit::returnPressed, this, [this]() { btn_search_->click(); });

    connect(tb_results_, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }
        const int row = index.row();
        if (row < 0 || row >= results_.size()) {
            return;
        }
        emit sgn_lyrics_selected(results_.at(row));
    });
}

void LyricsSearchWidget::update_result_table(const QVector<lyrics_fetcher::LyricMeta>& results)
{
    results_ = results;
    model_->removeRows(0, model_->rowCount());

    for (const auto& meta : results_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(meta.title);
        row << new QStandardItem(meta.artist);
        row << new QStandardItem(meta.album);
        row << new QStandardItem(meta.source);
        model_->appendRow(row);
    }

    if (results_.isEmpty()) {
        QMessageBox::information(this, tr("Lyrics Search"), tr("No lyrics result found."));
    }
}
