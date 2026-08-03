#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class TimeProgress : public QWidget
{
    Q_OBJECT;

public:
    explicit TimeProgress(QWidget* parent = nullptr);
    ~TimeProgress();

    void set_total_time(const qint64& time_s);
    void set_current_time(const qint64& time_s);

private:
    qint64 m_currentTime = 0;
    qint64 m_totalTime   = 0;
    QHBoxLayout* m_hbl_main;
    QLabel* m_lb_current_time;
    QLabel* m_lb_separator;
    QLabel* m_lb_total_time;

    QString format_time(const qint64& time_s);

    int m_hour_flag;
};
