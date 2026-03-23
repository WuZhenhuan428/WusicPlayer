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
    QListWidget *m_list_widget;
    QStackedWidget* m_stacked_widget;

    QHBoxLayout* m_hbl_settings;
    QHBoxLayout* m_hbl_bottom;
    QVBoxLayout* m_vbl_main;
    QPushButton* m_btn_close;
};