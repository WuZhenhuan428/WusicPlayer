#include "tag_edit_widget.h"

#include <QStandardItemModel>
#include <QHeaderView>

#include "core/utils/AudioUtils.h"

TagEditWidget::TagEditWidget(TrackMetaData meta, trackId tid, QWidget* parent)
    : QWidget(parent)
{
    this->initUI();
    this->initConnections();
    this->initTableModel(meta);
    m_le_filepath->setText(meta.filepath);
}

TagEditWidget::~TagEditWidget() {}

void TagEditWidget::initUI() {
    m_hbl_filepath = new QHBoxLayout();
    m_lb_filepath = new QLabel("path:", this);
    m_le_filepath = new QLineEdit(this);
    m_hbl_filepath->addWidget(m_lb_filepath);
    m_hbl_filepath->addWidget(m_le_filepath);

    m_table_metadata = new QTableView(this);

    m_btn_ok = new QPushButton("OK", this);
    m_btn_cancel = new QPushButton("Cancel", this);
    m_hbl_buttons = new QHBoxLayout();
    m_hbl_buttons->addStretch();
    m_hbl_buttons->addWidget(m_btn_ok);
    m_hbl_buttons->addWidget(m_btn_cancel);

    m_vbl_main = new QVBoxLayout();
    m_vbl_main->addLayout((m_hbl_filepath));
    m_vbl_main->addWidget(m_table_metadata);
    m_vbl_main->addLayout(m_hbl_buttons);

    this->setLayout(m_vbl_main);

    this->setMinimumSize(800, 600);

    m_le_filepath->setReadOnly(true);
    m_table_metadata->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table_metadata->horizontalHeader()->setStretchLastSection(true);
    m_table_metadata->verticalHeader()->setVisible(false);
}

void TagEditWidget::initConnections() {

}

void TagEditWidget::initTableModel(TrackMetaData meta) {

    m_meta_buffer.clear();
    m_meta_buffer = AudioUtils::parse_meta_to_map(meta.filepath.toStdString());

    auto firstValue = [&](std::initializer_list<QString> keys) -> QString {
        for (const QString& key : keys) {
            const QStringList values = m_meta_buffer.value(key);
            if (!values.isEmpty()) {
                return values.first().trimmed();
            }
        }
        return {};
    };

    QStandardItemModel* table_model = new QStandardItemModel();

    table_model->setColumnCount(2);
    table_model->setHeaderData(0, Qt::Horizontal, tr("Property"));
    table_model->setHeaderData(1, Qt::Horizontal, tr("Value"));

    this->m_table_metadata->setModel(table_model);
    this->m_table_metadata->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    int idx = 0;
    for (auto it = m_meta_buffer.begin(); it != m_meta_buffer.end(); ++it) {
        QStandardItem* prop = new QStandardItem(keyToName(it.key()));
        QStandardItem* value = new QStandardItem(firstValue({it.key()}));
        table_model->setItem(idx, 0, prop);
        table_model->setItem(idx, 1, value);
        idx++;
    }
}

QString TagEditWidget::keyToName(const QString& key) {
    /* key = TagLib property, value = name (QString) */
    static QMap<QString, QString> map = {
        {"ALBUM",       tr("album")},
        {"ALBUMARTIST", tr("album artist")},
        {"ARTIST",      tr("artist")},
        {"COMMENT",     tr("comment")},
        {"COMPOSER",    tr("COMPOSER")},
        {"DATE",        tr("date")},
        {"DISCNUMBER",  tr("disc number")},
        {"GENRE",       tr("genre")},
        {"LANGUAGE",    tr("language")},
        {"LENGTH",      tr("length")},
        {"LYRICS",      tr("lyrics")},
        {"TITLE",       tr("title")},
        {"TRACKNUMBER", tr("track number")}
    };

    if (map.contains(key)) {
        return map.value(key);
    } else {
        return QString("<%1>").arg(key);
    }
}