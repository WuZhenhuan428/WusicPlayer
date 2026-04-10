#include "tag_edit_widget.h"

#include <QHeaderView>
#include <QObject>
#include <QDebug>
#include <QRegularExpression>

#include "core/utils/AudioUtils.h"

TagEditWidget::TagEditWidget(TrackMetaData meta, trackId tid, QWidget* parent)
    : QWidget(parent),
      m_tid(tid)
{
    this->initUI();
    this->initUIProperties(meta.filepath);
    this->initConnections();
    this->initTableModel(meta);
}

TagEditWidget::~TagEditWidget() {}

void TagEditWidget::initUI() {
    m_hbl_filepath = new QHBoxLayout();
    m_lb_filepath = new QLabel("path: ", this);
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
}

void TagEditWidget::initUIProperties(const QString& filepath) {
    m_le_filepath->setText(filepath);
    m_le_filepath->setReadOnly(true);

    m_table_metadata->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table_metadata->horizontalHeader()->setStretchLastSection(true);
    m_table_metadata->verticalHeader()->setVisible(false);
}

void TagEditWidget::initConnections() {
    connect(m_btn_cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(m_btn_ok, &QPushButton::clicked, this, &TagEditWidget::handleSaveTags);
}

void TagEditWidget::handleSaveTags() {
    if (!m_table_model) {
        qDebug() << "[TagEdit]: table_model does not exist!";
        return;
    }

    auto toTagKey = [](const QString& input) -> QString {
        QString key = input.trimmed();
        if (key.startsWith('<') && key.endsWith('>') && key.size() > 2) {
            key = key.mid(1, key.size() - 2);
        }

        QString normalized;
        normalized.reserve(key.size());
        for (const QChar& ch : key) {
            if (ch == '_' || ch == '-' || ch == ' ') {
                continue;
            }
            normalized.append(ch.toUpper());
        }
        return normalized;
    };

    QMap<QString, QStringList> tag_buffer;
    for (int row = 0; row < m_table_model->rowCount(); ++row) {
        QModelIndex prop_index = m_table_model->index(row, 0);
        QModelIndex value_index = m_table_model->index(row, 1);

        QString prop_str = m_table_model->data(prop_index, Qt::UserRole + 1).toString();
        if (prop_str.isEmpty()) {
            prop_str = m_table_model->data(prop_index).toString();
        }

        const QString tag_key = toTagKey(prop_str);
        if (tag_key.isEmpty()) {
            continue;
        }

        QString value_str = m_table_model->data(value_index).toString();
        const QStringList parts = value_str.split(QRegularExpression("[,;]"), Qt::SkipEmptyParts);
        QStringList values;
        values.reserve(parts.size());
        for (const QString& item : parts) {
            const QString trimmed = item.trimmed();
            if (!trimmed.isEmpty()) {
                values << trimmed;
            }
        }

        tag_buffer.insert(tag_key, values);
    }

    emit sgnSaveTags(tag_buffer, m_tid);
    this->close();
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

    if (!m_table_model) {
        m_table_model = new QStandardItemModel();
    }
    m_table_model->clear();

    m_table_model->setColumnCount(2);
    m_table_model->setHeaderData(0, Qt::Horizontal, tr("Property"));
    m_table_model->setHeaderData(1, Qt::Horizontal, tr("Value"));

    this->m_table_metadata->setModel(m_table_model);
    this->m_table_metadata->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    int index = 0;
    for (auto it = m_meta_buffer.begin(); it != m_meta_buffer.end(); ++it) {
        QStandardItem* prop = new QStandardItem(keyToName(it.key()));
        prop->setData(it.key(), Qt::UserRole + 1);
        QStandardItem* value = new QStandardItem(firstValue({it.key()}));
        m_table_model->setItem(index, 0, prop);
        m_table_model->setItem(index, 1, value);
        index++;
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