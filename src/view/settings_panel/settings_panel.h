#pragma once

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHideEvent>
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
    ~SettingsPanel();

    void register_widget(QListWidgetItem* title, QWidget* widget);
    void switch_to_page_by_title(const QString& title);
    void emitStateSnapshot();

    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    QString config_sub_key() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;

signals:
    void sgnStateSnapshot(const QByteArray& geometry);

private:
    QListWidget* m_list_widget;
    QStackedWidget* m_stacked_widget;

    QHBoxLayout* m_hbl_settings;
    QHBoxLayout* m_hbl_bottom;
    QVBoxLayout* m_vbl_main;
    QPushButton* m_btn_close;

    ConfigManager* m_cfg_mgr;
};
