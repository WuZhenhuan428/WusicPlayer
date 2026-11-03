#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
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

protected:
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void sgn_result(const QPair<QString, QString>& result);

private:
    void init_ui();
    void init_connections();
    bool check_repetition();
    void save_result();

    // bool verify_prop(QString property);
private:
    QPair<QString, QString> result_;
    QStringList existed_props_;
    QStringList common_types_ = {
        "Album", "Album artist", "Artist", "Comment", "Composer", "Date",         "Disc number",
        "Genre", "Language",     "Length", "Lyrics",  "Title",    "Track number", "Other",
    };

private:
    // common properties ans "other"
    QLabel* lb_type_         = nullptr;
    QComboBox* cb_type_      = nullptr;
    QHBoxLayout* hbl_type_   = nullptr;

    // if m_cb_type = "other", enable m_le_other.
    QLabel* lb_other_        = nullptr;
    QLineEdit* le_other_     = nullptr;
    QHBoxLayout* hbl_other_  = nullptr;

    QLabel* lb_value_        = nullptr;
    QLineEdit* le_value_     = nullptr;
    QHBoxLayout* hbl_value_  = nullptr;

    QPushButton* btn_help_   = nullptr;
    QPushButton* btn_apply_  = nullptr;
    QPushButton* btn_cancel_ = nullptr;
    QHBoxLayout* hbl_btn_    = nullptr;

    QVBoxLayout* vbl_main_   = nullptr;
};
