#ifndef WTIMEPROGRESS_H
#define WTIMEPROGRESS_H

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
    qint64 totalTime;
    qint64 currTime;
    qint64 m_currentTime = 0;
    qint64 m_totalTime = 0;
    QHBoxLayout* layout;
    QLabel* lbCurrTime;
    QLabel* lbSeparator;
    QLabel* lbTotalTime;

    QString formatTime(const qint64& time_s);

    int hour_flag;
};

#endif // WTIMEPROGRESS_H
