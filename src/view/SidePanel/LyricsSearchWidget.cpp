#include "LyricsSearchWidget.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QThread>
#include <QMetaObject>

namespace {
static lyrics_fetcher::TrackMeta parseQueryToMeta(const QString& query) {
    lyrics_fetcher::TrackMeta meta;
    const QString trimmed = query.trimmed();
    const QStringList parts = trimmed.split(" - ", Qt::SkipEmptyParts);
    if (parts.size() >= 2) {
        meta.rawTitle = parts.at(0).trimmed();
        meta.rawArtist = parts.at(1).trimmed();
    } else {
        meta.rawTitle = trimmed;
        meta.rawArtist.clear();
    }
    return meta;
}
}

LyricsSearchWidget::LyricsSearchWidget(QWidget* parent)
    : QWidget(parent)
{
    initUi();
    initConnections();
}

void LyricsSearchWidget::setInitialQuery(const QString& title, const QString& artist) {
    const QString query = artist.trimmed().isEmpty()
        ? title.trimmed()
        : QString("%1 - %2").arg(title.trimmed(), artist.trimmed());
    m_le_query->setText(query.trimmed());
}

void LyricsSearchWidget::setSearchContext(const lyrics_fetcher::TrackMeta& context) {
    m_search_context = context;
}

void LyricsSearchWidget::initUi() {
    m_le_query = new QLineEdit(this);
    m_le_query->setPlaceholderText(tr("Input title or title - artist"));

    m_btn_search = new QPushButton(tr("Search"), this);

    m_tb_results = new QTableView(this);
    m_model = new QStandardItemModel(this);
    m_model->setColumnCount(4);
    m_model->setHeaderData(0, Qt::Horizontal, tr("Title"));
    m_model->setHeaderData(1, Qt::Horizontal, tr("Artist"));
    m_model->setHeaderData(2, Qt::Horizontal, tr("Album"));
    m_model->setHeaderData(3, Qt::Horizontal, tr("Source"));

    m_tb_results->setModel(m_model);
    m_tb_results->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tb_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tb_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tb_results->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tb_results->horizontalHeader()->setStretchLastSection(true);
    m_tb_results->verticalHeader()->setVisible(false);

    auto* hbl_top = new QHBoxLayout();
    hbl_top->addWidget(m_le_query);
    hbl_top->addWidget(m_btn_search);

    auto* vbl_main = new QVBoxLayout();
    vbl_main->addLayout(hbl_top);
    vbl_main->addWidget(m_tb_results);

    setLayout(vbl_main);
    setMinimumSize(640, 420);
    setWindowTitle(tr("Lyrics Search"));
}

void LyricsSearchWidget::initConnections() {
    connect(m_btn_search, &QPushButton::clicked, this, [this]() {
        if (m_searching) {
            return;
        }

        const QString query = m_le_query->text().trimmed();
        if (query.isEmpty()) {
            return;
        }

        m_searching = true;
        m_btn_search->setEnabled(false);
        m_model->removeRows(0, m_model->rowCount());
        m_results.clear();

        lyrics_fetcher::TrackMeta track_meta = parseQueryToMeta(query);
        if (track_meta.rawArtist.trimmed().isEmpty()) {
            track_meta.rawArtist = m_search_context.rawArtist;
        }
        if (track_meta.rawAlbum.trimmed().isEmpty()) {
            track_meta.rawAlbum = m_search_context.rawAlbum;
        }
        if (track_meta.durationSec <= 0) {
            track_meta.durationSec = m_search_context.durationSec;
        }
        QPointer<LyricsSearchWidget> self(this);

        QThread* worker = QThread::create([self, track_meta]() {
            QVector<lyrics_fetcher::LyricMeta> results;
            QNetworkAccessManager nam;

            lyrics_fetcher::LyricsManager manager;
            results = manager.fetch(track_meta, &nam);

            QMetaObject::invokeMethod(self, [self, results]() {
                if (!self) {
                    return;
                }
                self->updateResultTable(results);
                self->m_searching = false;
                self->m_btn_search->setEnabled(true);
            }, Qt::QueuedConnection);
        });

        connect(worker, &QThread::finished, worker, &QObject::deleteLater);
        worker->start();
    });

    connect(m_le_query, &QLineEdit::returnPressed, this, [this]() {
        m_btn_search->click();
    });

    connect(m_tb_results, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }
        const int row = index.row();
        if (row < 0 || row >= m_results.size()) {
            return;
        }
        emit sgnLyricSelected(m_results.at(row));
    });
}

void LyricsSearchWidget::updateResultTable(const QVector<lyrics_fetcher::LyricMeta>& results) {
    m_results = results;
    m_model->removeRows(0, m_model->rowCount());

    for (const auto& meta : m_results) {
        QList<QStandardItem*> row;
        row << new QStandardItem(meta.title);
        row << new QStandardItem(meta.artist);
        row << new QStandardItem(meta.album);
        row << new QStandardItem(meta.source);
        m_model->appendRow(row);
    }

    if (m_results.isEmpty()) {
        QMessageBox::information(this, tr("Lyrics Search"), tr("No lyrics result found."));
    }
}
