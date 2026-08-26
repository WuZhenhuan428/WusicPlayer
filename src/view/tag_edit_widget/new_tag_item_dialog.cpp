#include "new_tag_item_dialog.h"

#include <QMessageBox>

NewTagItemDialog::NewTagItemDialog(QStringList existed_props, QWidget* parent) :
    QWidget(parent), m_existed_props(existed_props)
{
    this->init_ui();
    this->init_connections();
}

NewTagItemDialog::~NewTagItemDialog() {}

void NewTagItemDialog::init_ui()
{
    m_lb_type  = new QLabel(tr("Type: "), this);
    m_cb_type  = new QComboBox(this);
    m_hbl_type = new QHBoxLayout;
    m_hbl_type->addWidget(m_lb_type);
    m_hbl_type->addWidget(m_cb_type);

    m_lb_other  = new QLabel(tr("Other: "), this);
    m_le_other  = new QLineEdit(this);
    m_hbl_other = new QHBoxLayout;
    m_hbl_other->addWidget(m_lb_other);
    m_hbl_other->addWidget(m_le_other);

    m_lb_value  = new QLabel(tr("value: "), this);
    m_le_value  = new QLineEdit(this);
    m_hbl_value = new QHBoxLayout;
    m_hbl_value->addWidget(m_lb_value);
    m_hbl_value->addWidget(m_le_value);

    m_btn_help   = new QPushButton("Help", this);
    m_btn_apply  = new QPushButton("Apply", this);
    m_btn_cancel = new QPushButton("Cancel", this);
    m_hbl_btn    = new QHBoxLayout;
    m_hbl_btn->addWidget(m_btn_help);
    m_hbl_btn->addStretch();
    m_hbl_btn->addWidget(m_btn_apply);
    m_hbl_btn->addWidget(m_btn_cancel);

    m_vbl_main = new QVBoxLayout;
    m_vbl_main->addLayout(m_hbl_type);
    m_vbl_main->addLayout(m_hbl_other);
    m_vbl_main->addLayout(m_hbl_value);
    m_vbl_main->addStretch();
    m_vbl_main->addLayout(m_hbl_btn);

    this->setLayout(m_vbl_main);

    m_cb_type->addItems(m_common_types);
    m_le_other->setEnabled(false);
}

void NewTagItemDialog::init_connections()
{
    connect(m_cb_type, &QComboBox::currentTextChanged, this, [this](const QString& str) {
        QString key = name_to_key(str);
        if (key == "OTHER") {
            m_le_other->setEnabled(true);
            m_le_other->setFocus();
        } else {
            m_le_other->setEnabled(false);
            m_le_other->clear();
        }
    });

    connect(m_btn_help, &QPushButton::clicked, this, [this]() {
        QMessageBox* msb = new QMessageBox(this);
        msb->setWindowTitle(tr("Help"));
        msb->setText(tr("This is help message box about\nrules of create new tag"));
        msb->setIcon(QMessageBox::Icon::Information);
        msb->setStandardButtons(QMessageBox::Ok);
        msb->setAttribute(Qt::WA_DeleteOnClose);
        msb->show();
    });

    connect(m_btn_apply, &QPushButton::clicked, this, [this]() {
        if (check_repetition()) {
            save_result();
            emit sgnResult(m_result);
            this->close();
        } else {
            QMessageBox* msb = new QMessageBox(this);
            msb->setWindowTitle(tr("Warning"));
            msb->setText(tr("Please choose/input correct type of tag!"));
            msb->setIcon(QMessageBox::Icon::Warning);
            msb->setStandardButtons(QMessageBox::Ok);
            msb->setAttribute(Qt::WA_DeleteOnClose);
            msb->show();
        }
    });

    connect(m_btn_cancel, &QPushButton::clicked, this, [this]() { this->close(); });
}

bool NewTagItemDialog::check_repetition()
{
    QString current_key = name_to_key(m_cb_type->currentText());

    if (m_existed_props.contains(current_key)) {
        return false;
    } else if (current_key == "OTHER") {
        QString key = m_le_other->text().trimmed();
        // if other type in common types
        if (m_existed_props.contains(name_to_key(key)) || key.isEmpty()) {
            return false;
        }
    }
    // allow key-only prop, so remove value check

    return true;
}

void NewTagItemDialog::save_result()
{
    if (name_to_key(m_cb_type->currentText()) == "OTHER") {
        m_result.first = name_to_key(m_le_other->text());
    } else {
        m_result.first = name_to_key(m_cb_type->currentText());
    }
    m_result.second = m_le_value->text().trimmed();
}

QString NewTagItemDialog::name_to_key(const QString& name)
{
    // stolen from TagEditWidget ^^;
    QString trimmed = name.trimmed();

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
