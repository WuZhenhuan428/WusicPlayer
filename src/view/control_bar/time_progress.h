#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class TimeProgress : public QWidget
{
    Q_OBJECT;

public:
    explicit TimeProgress(QWidget* parent = nullptr);

    void set_total_time(const qint64& time_s);
    void set_current_time(const qint64& time_s);

private:
    qint64 current_time_ = 0;
    qint64 total_time_   = 0;
    QHBoxLayout* hbl_main_;
    QLabel* lb_current_time_;
    QLabel* lb_separator_;
    QLabel* lb_total_time_;

    static QString format_time(const qint64& time_s);

    int hour_flag_;
};
