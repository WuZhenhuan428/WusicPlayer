#pragma once

#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QHideEvent>

class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(QWidget *parent = nullptr);
    ~SettingsPanel();

    void registerWidget(QListWidgetItem* title, QWidget* widget);
    void emitStateSnapshot();

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;

signals:
    void sgnStateSnapshot(const QByteArray& geometry);

private:
    QListWidget *m_listWidget;
    QStackedWidget* m_stackedWidget;

    QHBoxLayout* m_settingsLayout;
    QHBoxLayout* m_bottomLayout;
    QVBoxLayout* m_mainLayout;
    QPushButton* m_btnClose;
};