#include "status_bar_controller.h"

#include <QPointer>

StatusBarController::StatusBarController(QStatusBar* status_bar, QObject* parent) : QObject(parent)
{
    this->status_bar_ = status_bar;
}

void StatusBarController::register_item(const QString& id, const QString& default_text)
{
    QLabel* lb = new QLabel(default_text, this->status_bar_);
    lb->setObjectName("status_" + id);
    this->status_bar_->addWidget(lb);
}

QLabel* StatusBarController::find_label(const QString& id)
{
    QLabel* lb = this->status_bar_->findChild<QLabel*>("status_" + id, Qt::FindDirectChildrenOnly);
    if (lb) {
        return lb;
    }
    return nullptr;
}

bool StatusBarController::update_item_by_id(const QString& id, const QString& text)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->setText(text);
        return true;
    }
    return false;
}

bool StatusBarController::remove_item_by_id(const QString& id)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        this->status_bar_->removeWidget(lb);
        return true;
    }
    return false;
}
