#pragma once

#include "core/types.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QString>
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
public:
    explicit WSortTypeSetDialog(QDialog* parent = nullptr);
    ~WSortTypeSetDialog();

    QString getText() const
    {
        return txtExpression->text();
    }

private:
    QLabel* lbPrompt;
    QLineEdit* txtExpression;

    QPushButton* btnEnter;
    QPushButton* btnCancel;
    QPushButton* btnHelp;

    QSlider* sldMain;
    QSlider* sldBtn;
    QHBoxLayout* hbPrompt;
    QHBoxLayout* hbBtn;
    QVBoxLayout* vbMain;
};

class WPlayListWidgetItem : public QTreeWidgetItem
{
public:
    WPlayListWidgetItem(QTreeWidget* parent, const QString& name, const PlaylistId& pid) :
        QTreeWidgetItem(parent, QStringList() << name), m_pid(pid)
    {}

    PlaylistId id() const
    {
        return m_pid;
    }

private:
    PlaylistId m_pid;
};

class WInsertColumnDialog : public QDialog
{
public:
    explicit WInsertColumnDialog();
    ~WInsertColumnDialog();
    TableColumn get_rule();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lbIndex;
    QLineEdit* txtIndex;
    QHBoxLayout* hbIndex;
    int m_maxIndex = 0;

    QLabel* lbTitle;
    QLineEdit* txtTitle;
    QHBoxLayout* hbTitle;

    QLabel* lbType;
    QComboBox* cbType;
    QHBoxLayout* hbType;

    QPushButton* btnOK;
    QPushButton* btnCancel;
    QHBoxLayout* hbBtn;

    QVBoxLayout* vbMain;
};

class WColumnIndexDialog : public QDialog
{
public:
    explicit WColumnIndexDialog(const QString& title, const QString& prompt,
                                QWidget* parent = nullptr);
    ~WColumnIndexDialog();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lbPrompt;
    QLineEdit* txtIndex;
    QHBoxLayout* hbIndex;
    int m_maxIndex = 0;

    QPushButton* btnOK;
    QPushButton* btnCancel;
    QHBoxLayout* hbBtn;

    QVBoxLayout* vbMain;
};
