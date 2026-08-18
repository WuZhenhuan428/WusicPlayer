#pragma once

#include <QLabel>
#include <QObject>
#include <QSet>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QVector>

class StatusBarController : public QObject
{
public:
    explicit StatusBarController(QStatusBar* status_bar, QObject* parent = nullptr);
    ~StatusBarController() = default;

    void register_item(const QString& id, const QString& defalt_text = "");
    bool update_item_by_id(const QString& id, const QString& text);
    bool show_item_by_id(const QString& id);
    bool hide_item_by_id(const QString& id);
    bool remove_item_by_id(const QString& id);

    void show_temp_message(const QString& message, int ms);

private:
    QLabel* find_label(const QString& id);

    QStatusBar* status_bar_ = nullptr; // 来自 MainWindow
    QLabel* lb_temp_msg_;              // 持有
    QTimer* timer_;                    // 持有
    int permanent_widget_cnt_ = 0;
};
