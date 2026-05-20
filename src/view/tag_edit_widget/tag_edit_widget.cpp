#include "tag_edit_widget.h"

#include <QHeaderView>
#include <QObject>
#include <QDebug>
#include <QRegularExpression>
#include <QPoint>
#include <QModelIndex>
#include <QMenu>
#include <QAction>
#include <QSet>

#include <QMessageBox>

#include "core/utils/AudioUtils.h"

#include "view/dialogs/text_editor_dialog.h"
#include "new_tag_item_dialog.h"


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
    m_lb_filepath = new QLabel("Location: ", this);
    m_le_filepath = new QLineEdit(this);
    m_hbl_filepath->addWidget(m_lb_filepath);
    m_hbl_filepath->addWidget(m_le_filepath);

    m_table_metadata = new QTableView(this);

    m_btn_help = new QPushButton("Help", this);
    m_btn_ok = new QPushButton("OK", this);
    m_btn_cancel = new QPushButton("Cancel", this);
    m_hbl_buttons = new QHBoxLayout();
    m_hbl_buttons->addWidget(m_btn_help);
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
    m_table_metadata->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table_metadata->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table_metadata->verticalHeader()->setVisible(false);
}

void TagEditWidget::initConnections() {
    connect(m_btn_help, &QPushButton::clicked, this, [this](){
        QMessageBox* msb = new QMessageBox(this);
        msb->setWindowTitle("Help");
        msb->setText("This is help message box");
        msb->setIcon(QMessageBox::Icon::Information);
        msb->setStandardButtons(QMessageBox::Ok);
        msb->setAttribute(Qt::WA_DeleteOnClose);
        msb->show();
    });
    connect(m_btn_cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(m_btn_ok, &QPushButton::clicked, this, &TagEditWidget::handleSaveTags);

    connect(m_table_metadata, &QTableView::customContextMenuRequested, this, &TagEditWidget::handleShowMenu);
}

void TagEditWidget::handleShowMenu(const QPoint& pos) {
    QModelIndex index = this->m_table_metadata->indexAt(pos);
    QMenu menu(this);
    if (index.isValid()) {  // right click on item: edit or delete
        index = index.sibling(index.row(), 1);  // move to value
        
        QAction* act_edit = menu.addAction("Edit");
        QAction* act_delete = menu.addAction("Delete");

        connect(act_edit, &QAction::triggered, this, [this, index](){
            handleEditItem(index);
        });
        connect(act_delete, &QAction::triggered, this, [this, index](){
            handleDeleteItem(index);
        });
    }
    
    // all cases:
    QAction* act_add = menu.addAction("Add new filed");

    connect(act_add, &QAction::triggered, this, [this](){
        handleAddNewFiled();
    });
    menu.exec(this->m_table_metadata->viewport()->mapToGlobal(pos));
}

void TagEditWidget::handleSaveTags() {
    if (!m_table_model) {
        qDebug() << "[TagEdit]: table_model does not exist!";
        return;
    }

    QMap<QString, QStringList> tag_buffer;
    QSet<QString> current_keys;
    for (int row = 0; row < m_table_model->rowCount(); ++row) {
        QModelIndex prop_index = m_table_model->index(row, 0);
        QModelIndex value_index = m_table_model->index(row, 1);

        QString prop_str = m_table_model->data(prop_index, Qt::UserRole + 1).toString();
        if (prop_str.isEmpty()) {
            prop_str = m_table_model->data(prop_index).toString();
        }

        const QString tag_key = nameToKey(prop_str);
        if (tag_key.isEmpty()) {
            continue;
        }

        current_keys.insert(tag_key);

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

    // Explicitly submit removed keys so writeback can erase them from file tags.
    QSet<QString> original_keys;
    for (auto it = m_meta_buffer.constBegin(); it != m_meta_buffer.constEnd(); ++it) {
        const QString key = nameToKey(it.key());
        if (!key.isEmpty()) {
            original_keys.insert(key);
        }
    }

    const QSet<QString> deleted_keys = original_keys - current_keys;
    for (const QString& key : deleted_keys) {
        tag_buffer.insert(key, QStringList{});
    }

    emit sgnSaveTags(tag_buffer, m_tid);
    this->close();
}

void TagEditWidget::handleEditItem(QModelIndex index) {
    if (!m_table_model || !index.isValid()) {
        return;
    }

    const QModelIndex prop_index = index.sibling(index.row(), 0);
    const QModelIndex value_index = index.sibling(index.row(), 1);

    QString prop_key = m_table_model->data(prop_index, Qt::UserRole + 1).toString();
    if (prop_key.isEmpty()) {
        prop_key = m_table_model->data(prop_index, Qt::DisplayRole).toString();
    }

    if (nameToKey(prop_key) == "LYRICS") {
        const QString text = m_table_model->data(value_index, Qt::DisplayRole).toString();

        TextEditorDialog* dialog = new TextEditorDialog(text, this);
        dialog->setWindowFlag(Qt::Window, true);
        dialog->setWindowTitle(tr("Edit Lyrics"));
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        connect(dialog, &TextEditorDialog::sgnText, this, [this, value_index](const QString& new_text){
            m_table_model->setData(value_index, new_text, Qt::DisplayRole);
        });

        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        return;
    }

    m_table_metadata->edit(value_index);
}

void TagEditWidget::handleDeleteItem(QModelIndex index) {
    this->m_table_model->removeRow(index.row());
    this->m_table_metadata->update();
}

void TagEditWidget::handleAddNewFiled() {
    // open a new dialog, select a default property.
    // if select `other`, then input title in a QLineEdit
    // then press `OK` button to create new item
    QStringList list;
    for (int i = 0; i < m_table_model->rowCount(); ++i) {
        QString key = nameToKey(m_table_model->item(i)->data().toString());
        list << key;
        // qDebug() << key; // OK
    }
    NewTagItemDialog* widget = new NewTagItemDialog(list, this);

    widget->setWindowFlag(Qt::Window, true);
    widget->setWindowTitle(tr("Add New Tag Item"));
    widget->setAttribute(Qt::WA_DeleteOnClose);

    connect(widget, &NewTagItemDialog::sgnResult, this, [this](const QPair<QString, QString>& result){
        if (!m_table_model) {
            return;
        }

        const QString key = nameToKey(result.first);
        if (key.isEmpty()) {
            return;
        }

        for (int row = 0; row < m_table_model->rowCount(); ++row) {
            const QString existing_key = nameToKey(m_table_model->item(row, 0)->data(Qt::UserRole + 1).toString());
            if (existing_key == key) {
                return;
            }
        }

        QStandardItem* prop = new QStandardItem(keyToName(key));
        prop->setData(key, Qt::UserRole + 1);
        prop->setEditable(false);

        QStandardItem* value = new QStandardItem(result.second);

        const int row = m_table_model->rowCount();
        m_table_model->setItem(row, 0, prop);
        m_table_model->setItem(row, 1, value);

        QModelIndex value_index = m_table_model->index(row, 1);
        m_table_metadata->setCurrentIndex(value_index);
        m_table_metadata->scrollTo(value_index, QAbstractItemView::PositionAtCenter);
    });

    widget->show();
    widget->raise();
    widget->activateWindow();
    return;
}


void TagEditWidget::initTableModel(TrackMetaData meta) {

    m_meta_buffer.clear();
    m_meta_buffer = AudioUtils::parse_meta_to_map(meta.filepath);

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
        prop->setEditable(false);
        QStandardItem* value = new QStandardItem(firstValue({it.key()}));
        m_table_model->setItem(index, 0, prop);
        m_table_model->setItem(index, 1, value);
        index++;
    }
}

QString TagEditWidget::keyToName(const QString& key) {
    /* key = TagLib property, value = name (QString) */
    static QMap<QString, QString> map = {
        {"ALBUM",       tr("Album")},
        {"ALBUMARTIST", tr("Album artist")},
        {"ARTIST",      tr("Artist")},
        {"COMMENT",     tr("Comment")},
        {"COMPOSER",    tr("Composer")},
        {"DATE",        tr("Date")},
        {"DISCNUMBER",  tr("Disc number")},
        {"GENRE",       tr("Genre")},
        {"LANGUAGE",    tr("Language")},
        {"LENGTH",      tr("Length")},
        {"LYRICS",      tr("Lyrics")},
        {"TITLE",       tr("Title")},
        {"TRACKNUMBER", tr("Track number")}
    };

    if (map.contains(key)) {
        return map.value(key);
    } else {
        return QString("<%1>").arg(key);
    }
}

QString TagEditWidget::nameToKey(const QString& name) {
    QString trimmed = name.trimmed();
    if (trimmed.startsWith('<') && trimmed.endsWith('>') && trimmed.size() > 2) {
        trimmed = trimmed.mid(1, trimmed.size() - 2);
    }
    QString normalized;
    normalized.reserve(trimmed.size());
    for (const QChar& ch : trimmed) {
        if (ch == '_' || ch == '-' || ch == ' ') {
            continue;
        }
        normalized.append(ch.toUpper());
    }
    return normalized;
}