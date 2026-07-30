#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

class StatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent);
    ~StatusBar() = default;

    void register_item(const QString& id, const QString& defalt_text = "");
    bool update_item_by_id(const QString& id, const QString& text);
    bool show_item_by_id(QString id);
    bool hide_item_by_id(QString id);
    bool remove_item_by_id(QString id);

private:
    void init_ui();
    inline QLabel* find_label(const QString& id);

    QSet<QLabel*> items_;
    QLabel* lb_place_holder_;
    QHBoxLayout* hbl_main_;
};
