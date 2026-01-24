#include "wtimeprogress.h"

WTimeProgress::WTimeProgress(QWidget *parent)
    : QWidget(parent)
{
    hour_flag = 0;
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 0, 5, 0); // direction: ltrd
    layout->setSpacing(5);

    // format: mm:ss/hh:ss
    lbCurrTime = new QLabel("00:00:00", this);
    lbSeparator = new QLabel("/", this);
    lbTotalTime= new QLabel("00:00:00", this);

    // Initialization
    setCurrentTime(0);
    lbSeparator->setText("/");
    setTotalTime(0);

    // Combine as a widget
    layout->addWidget(lbCurrTime);
    layout->addWidget(lbSeparator);
    layout->addWidget(lbTotalTime);

    setLayout(layout);
}

WTimeProgress::~WTimeProgress() {}

void WTimeProgress::setTotalTime(const qint64& time_s) {
    m_totalTime = time_s;
    lbTotalTime->setText(formatTime(time_s));
}

void WTimeProgress::setCurrentTime(const qint64& time_s) {
    m_currentTime = time_s;
    lbCurrTime->setText(formatTime(time_s));
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
