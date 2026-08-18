#include "controller/status_bar_controller.h"

#include <QPointer>

StatusBarController::StatusBarController(QStatusBar* status_bar, QObject* parent) : QObject(parent)
{
    this->status_bar_  = status_bar;
    this->lb_temp_msg_ = new QLabel(this->status_bar_);
    this->lb_temp_msg_->setAlignment(Qt::AlignRight);
    status_bar->addPermanentWidget(lb_temp_msg_, 1);
    permanent_widget_cnt_ += 1;

    this->timer_ = new QTimer(this);
    this->timer_->setSingleShot(true);
    connect(this->timer_, &QTimer::timeout, this, [this]() { this->lb_temp_msg_->clear(); });

    this->show_temp_message("Loading", 5000);
}

void StatusBarController::register_item(const QString& id, const QString& default_message)
{
    int total      = this->status_bar_->children().count();
    int insert_pos = total - this->permanent_widget_cnt_;
    QLabel* lb     = new QLabel(default_message, this->status_bar_);
    lb->setObjectName("status_" + id);
    this->status_bar_->insertWidget(insert_pos, lb);
}

QLabel* StatusBarController::find_label(const QString& id)
{
    QLabel* lb = this->status_bar_->findChild<QLabel*>("status_" + id, Qt::FindDirectChildrenOnly);
    if (lb) {
        return lb;
    }
    return nullptr;
}

bool StatusBarController::update_item_by_id(const QString& id, const QString& message)
{
    QLabel* lb = this->find_label(id);
    if (lb) {
        lb->setText(message);
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

void StatusBarController::show_temp_message(const QString& message, int ms)
{
    if (message.isEmpty()) {
        return;
    }
    this->lb_temp_msg_->setText(message);
    this->timer_->start(ms);
}

void StatusBarController::show_notification(const AppNotification& n)
{
    this->show_temp_message(n.message, n.duration_ms);
}

void StatusBarController::clear_notification()
{
    this->lb_temp_msg_->clear();
}
