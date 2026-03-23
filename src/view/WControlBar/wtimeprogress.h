#pragma once

#include <QWidget>

#include <QHBoxLayout>
#include <QLabel>

class WTimeProgress : public QWidget
{
    Q_OBJECT;
public:
    explicit WTimeProgress(QWidget *parent = nullptr);
    ~WTimeProgress();

    void setTotalTime(const qint64& time_s);
    void setCurrentTime(const qint64& time_s);
private:
    qint64 m_currentTime = 0;
    qint64 m_totalTime = 0;
    QHBoxLayout* m_hbl_main;
    QLabel* m_lb_current_time;
    QLabel* m_lb_separator;
    QLabel* m_lb_total_time;

    QString formatTime(const qint64& time_s);

    int m_hour_flag;
};
