#include "status_bar.h"

StatusBar::StatusBar(QWidget* parent) : QWidget(parent)
{
    this->init_ui();
}

void StatusBar::init_ui()
{
    this->lb_place_holder_ = new QLabel(" ", this);

    this->hbl_main_        = new QHBoxLayout;
    hbl_main_->setAlignment(Qt::AlignLeft);
    hbl_main_->addStretch();
}

void StatusBar::register_item(const QString& id, const QString& default_text)
{
    QLabel* lb = new QLabel(default_text, this);
    lb->setObjectName("status_" + id);
    lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->hbl_main_->insertWidget(hbl_main_->count() - 1, lb);
}

bool StatusBar::update_item_by_id(const QString& id, const QString& text)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->setText(text);
        return true;
    }
    return false;
}

QLabel* StatusBar::find_label(const QString& id)
{
    QLabel* lb = this->findChild<QLabel*>("status_" + id, Qt::FindDirectChildrenOnly);
    if (lb) {
        return lb;
    }
    return nullptr;
}

bool StatusBar::show_item_by_id(QString id)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->show();
    }
    return false;
}

bool StatusBar::hide_item_by_id(QString id)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->hide();
    }
    return false;
}

bool StatusBar::remove_item_by_id(QString id)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->deleteLater();
    }
    return false;
}
