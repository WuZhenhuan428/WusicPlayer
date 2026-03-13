#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTableView>
#include <QPushButton>

#include <QHBoxLayout>

class ShortcutsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ShortcutsPanel(QWidget* parent = nullptr);
    ~ShortcutsPanel();

    void setViewModel(QAbstractTableModel* model);

signals:
    void sgnApplyConfig();
    void sgnDefaultConfig();
    void sgnRestoreConfig();

private:
    QLabel* m_labelSearch;
    QLineEdit* m_lineEditSearch;
    QTableView* m_viewShortcuts;
    QPushButton* m_btnApply;
    QPushButton* m_btnDefault;
    QPushButton* m_btnRestore;
    QHBoxLayout* m_searchLineLayout;
    QHBoxLayout* m_buttomLayout;
    QVBoxLayout* m_mainLayout;
};