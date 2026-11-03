#pragma once

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class QJsonObject;
class ConfigManager;

class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(ConfigManager* cfg_mgr, QWidget* parent = nullptr);

    void register_widget(QListWidgetItem* title, QWidget* widget);
    void switch_to_page_by_title(const QString& title);

    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    static QString config_sub_key();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;

signals:
    void sgn_state_snapshot(const QByteArray& geometry);

private:
    QListWidget* list_widget_;
    QStackedWidget* stacked_widget_;

    QHBoxLayout* hbl_settings_;
    QHBoxLayout* hbl_bottom_;
    QVBoxLayout* vbl_main_;
    QPushButton* btn_close_;

    ConfigManager* cfg_mgr_;
};
