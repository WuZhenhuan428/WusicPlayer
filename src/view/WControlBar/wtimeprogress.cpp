#include "wtimeprogress.h"

WTimeProgress::WTimeProgress(QWidget *parent)
    : QWidget(parent)
{
    m_hour_flag = 0;
    m_hbl_main = new QHBoxLayout(this);
    m_hbl_main->setContentsMargins(5, 0, 5, 0); // direction: ltrd
    m_hbl_main->setSpacing(5);

    // format: mm:ss/hh:ss
    m_lb_current_time = new QLabel("00:00:00", this);
    m_lb_separator = new QLabel("/", this);
    m_lb_total_time= new QLabel("00:00:00", this);

    // Initialization
    setCurrentTime(0);
    m_lb_separator->setText("/");
    setTotalTime(0);

    // Combine as a widget
    m_hbl_main->addWidget(m_lb_current_time);
    m_hbl_main->addWidget(m_lb_separator);
    m_hbl_main->addWidget(m_lb_total_time);

    setLayout(m_hbl_main);
}

WTimeProgress::~WTimeProgress() {}

void WTimeProgress::setTotalTime(const qint64& time_s) {
    m_totalTime = time_s;
    m_lb_total_time->setText(formatTime(time_s));
}

void WTimeProgress::setCurrentTime(const qint64& time_s) {
    m_currentTime = time_s;
    m_lb_current_time->setText(formatTime(time_s));
}

// time_s -> "hh:mm:ss"
QString WTimeProgress::formatTime(const qint64& time_s) {
    int hour = time_s/3600;
    int min = (time_s % 3600) / 60;
    int sec = time_s%60;
    return QString("%1:%2:%3")
        .arg(hour, 2, 10, QChar('0'))
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'));
}
