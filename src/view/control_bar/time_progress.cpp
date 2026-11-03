#include "time_progress.h"

TimeProgress::TimeProgress(QWidget* parent) : QWidget(parent)
{
    hour_flag_ = 0;
    hbl_main_  = new QHBoxLayout(this);
    hbl_main_->setContentsMargins(5, 0, 5, 0); // direction: ltrd
    hbl_main_->setSpacing(5);

    // format: mm:ss/hh:ss
    lb_current_time_ = new QLabel("00:00:00", this);
    lb_separator_    = new QLabel("/", this);
    lb_total_time_   = new QLabel("00:00:00", this);

    // Initialization
    set_current_time(0);
    lb_separator_->setText("/");
    set_total_time(0);

    // Combine as a widget
    hbl_main_->addWidget(lb_current_time_);
    hbl_main_->addWidget(lb_separator_);
    hbl_main_->addWidget(lb_total_time_);

    setLayout(hbl_main_);
}

void TimeProgress::set_total_time(const qint64& time_s)
{
    total_time_ = time_s;
    lb_total_time_->setText(format_time(time_s));
}

void TimeProgress::set_current_time(const qint64& time_s)
{
    current_time_ = time_s;
    lb_current_time_->setText(format_time(time_s));
}

// time_s -> "hh:mm:ss"
QString TimeProgress::format_time(const qint64& time_s)
{
    int hour = static_cast<int>(time_s / 3600);
    int min  = static_cast<int>(time_s % 3600) / 60;
    int sec  = static_cast<int>(time_s % 60);
    return QString("%1:%2:%3")
        .arg(hour, 2, 10, QChar('0'))
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'));
}
