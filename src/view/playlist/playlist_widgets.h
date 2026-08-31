#pragma once

#include "core/types.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QVBoxLayout>
#include <QVector>

/* usage:
    WSortTypeSetter dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        // process input text here
    }
*/
class WSortTypeSetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WSortTypeSetDialog(QWidget* parent = nullptr);
    ~WSortTypeSetDialog();

    /// 当前输入的 DSL 文本(多行)。
    QString getText() const
    {
        return txt_expression_->toPlainText();
    }
    /// 预填 DSL 文本(编辑既有规则)。
    void setText(const QString& text)
    {
        txt_expression_->setPlainText(text);
    }

private:
    QHBoxLayout* hbl_prompt_;
    QLabel* lb_prompt_;
    QTextEdit* txt_expression_;

    QHBoxLayout* hbl_btn_;
    QPushButton* btn_enter_;
    QPushButton* btn_cancel_;
    QPushButton* btn_help_;

    QVBoxLayout* vbl_main_;
};

class WPlayListWidgetItem : public QTreeWidgetItem
{
public:
    WPlayListWidgetItem(QTreeWidget* parent, const QString& name, const PlaylistId& pid) :
        QTreeWidgetItem(parent, QStringList() << name), pid_(pid)
    {
        // 支持"选中后单击"内联重命名(SelectedClicked 编辑触发器)
        setFlags(flags() | Qt::ItemIsEditable);
    }

    PlaylistId id() const
    {
        return pid_;
    }

private:
    PlaylistId pid_;
};

class WInsertColumnDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WInsertColumnDialog();
    ~WInsertColumnDialog();
    TableColumn get_rule();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lb_index_;
    QLineEdit* le_index_;
    QHBoxLayout* hbl_index_;
    int max_index_ = 0;

    QLabel* lb_title_;
    QLineEdit* le_title_;
    QHBoxLayout* hbl_title_;

    QLabel* lb_type_;
    QComboBox* cb_type_;
    QHBoxLayout* hbl_type_;

    QPushButton* btn_ok_;
    QPushButton* btn_cancel_;
    QHBoxLayout* hbl_btn_;

    QVBoxLayout* vbl_main_;
};

class WColumnIndexDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WColumnIndexDialog(const QString& title, const QString& prompt,
                                QWidget* parent = nullptr);
    ~WColumnIndexDialog();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lb_prompt_;
    QLineEdit* txt_index_;
    QHBoxLayout* hbl_index_;
    int max_index_ = 0;

    QPushButton* btn_ok_;
    QPushButton* btn_cancel_;
    QHBoxLayout* hbl_btn_;

    QVBoxLayout* vbl_main_;
};
