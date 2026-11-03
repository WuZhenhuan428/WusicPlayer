#include "new_tag_item_dialog.h"

#include <QMessageBox>

namespace
{
QString name_to_key(const QString& name)
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
} // namespace

NewTagItemDialog::NewTagItemDialog(QStringList existed_props, QWidget* parent) :
    QWidget(parent), existed_props_(std::move(existed_props))
{
    this->init_ui();
    this->init_connections();
}

void NewTagItemDialog::init_ui()
{
    lb_type_  = new QLabel(tr("Type: "), this);
    cb_type_  = new QComboBox(this);
    hbl_type_ = new QHBoxLayout;
    hbl_type_->addWidget(lb_type_);
    hbl_type_->addWidget(cb_type_);

    lb_other_  = new QLabel(tr("Other: "), this);
    le_other_  = new QLineEdit(this);
    hbl_other_ = new QHBoxLayout;
    hbl_other_->addWidget(lb_other_);
    hbl_other_->addWidget(le_other_);

    lb_value_  = new QLabel(tr("value: "), this);
    le_value_  = new QLineEdit(this);
    hbl_value_ = new QHBoxLayout;
    hbl_value_->addWidget(lb_value_);
    hbl_value_->addWidget(le_value_);

    btn_help_   = new QPushButton("Help", this);
    btn_apply_  = new QPushButton("Apply", this);
    btn_cancel_ = new QPushButton("Cancel", this);
    hbl_btn_    = new QHBoxLayout;
    hbl_btn_->addWidget(btn_help_);
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_apply_);
    hbl_btn_->addWidget(btn_cancel_);

    vbl_main_ = new QVBoxLayout;
    vbl_main_->addLayout(hbl_type_);
    vbl_main_->addLayout(hbl_other_);
    vbl_main_->addLayout(hbl_value_);
    vbl_main_->addStretch();
    vbl_main_->addLayout(hbl_btn_);

    this->setLayout(vbl_main_);

    cb_type_->addItems(common_types_);
    le_other_->setEnabled(false);
}

void NewTagItemDialog::init_connections()
{
    connect(cb_type_, &QComboBox::currentTextChanged, this, [this](const QString& str) {
        QString key = name_to_key(str);
        if (key == "OTHER") {
            le_other_->setEnabled(true);
            le_other_->setFocus();
        } else {
            le_other_->setEnabled(false);
            le_other_->clear();
        }
    });

    connect(btn_help_, &QPushButton::clicked, this, [this]() {
        QMessageBox* msb = new QMessageBox(this);
        msb->setWindowTitle(tr("Help"));
        msb->setText(tr("This is help message box about\nrules of create new tag"));
        msb->setIcon(QMessageBox::Icon::Information);
        msb->setStandardButtons(QMessageBox::Ok);
        msb->setAttribute(Qt::WA_DeleteOnClose);
        msb->show();
    });

    connect(btn_apply_, &QPushButton::clicked, this, [this]() {
        if (check_repetition()) {
            save_result();
            emit sgn_result(result_);
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

    connect(btn_cancel_, &QPushButton::clicked, this, [this]() { this->close(); });
}

bool NewTagItemDialog::check_repetition()
{
    QString current_key = name_to_key(cb_type_->currentText());

    if (existed_props_.contains(current_key)) {
        return false;
    }
    if (current_key == "OTHER") {
        QString key = le_other_->text().trimmed();
        // if other type in common types
        if (existed_props_.contains(name_to_key(key)) || key.isEmpty()) {
            return false;
        }
    }
    // allow key-only prop, so remove value check

    return true;
}

void NewTagItemDialog::save_result()
{
    if (name_to_key(cb_type_->currentText()) == "OTHER") {
        result_.first = name_to_key(le_other_->text());
    } else {
        result_.first = name_to_key(cb_type_->currentText());
    }
    result_.second = le_value_->text().trimmed();
}

void NewTagItemDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
}
