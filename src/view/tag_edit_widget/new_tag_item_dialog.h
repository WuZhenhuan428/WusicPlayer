#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPair>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

class NewTagItemDialog : public QWidget
{
    Q_OBJECT

public:
    explicit NewTagItemDialog(QStringList existed_props, QWidget* parent = nullptr);
    ~NewTagItemDialog();

signals:
    void sgnResult(const QPair<QString, QString>& result);

private:
    void initUI();
    void initConnections();
    bool checkRepetition();
    QString nameToKey(const QString& name);
    void saveResult();

    // bool verify_prop(QString property);
private:
    QPair<QString, QString> m_result;
    QStringList m_existed_props;
    QStringList m_common_types = {"Album",  "Album artist", "Artist",       "Comment",  "Composer",
                                  "Date",   "Disc number",  "Genre",        "Language", "Length",
                                  "Lyrics", "Title",        "Track number", "Other"};

private:
    // common properties ans "other"
    QLabel* m_lb_type;
    QComboBox* m_cb_type;
    QHBoxLayout* m_hbl_type;

    // if m_cb_type = "other", enable m_le_other.
    QLabel* m_lb_other;
    QLineEdit* m_le_other;
    QHBoxLayout* m_hbl_other;

    QLabel* m_lb_value;
    QLineEdit* m_le_value;
    QHBoxLayout* m_hbl_value;

    QPushButton* m_btn_help;
    QPushButton* m_btn_apply;
    QPushButton* m_btn_cancel;
    QHBoxLayout* m_hbl_btn;

    QVBoxLayout* m_vbl_main;
};
