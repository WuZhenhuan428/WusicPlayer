#pragma once

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QPushButton>
#include <QTableView>
#include <QWidget>

class QJsonObject;
class ConfigManager;

class ShortcutsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ShortcutsPanel(ConfigManager* cfg_mgr, QWidget* parent = nullptr);
    ~ShortcutsPanel();

    void set_view_model(QAbstractTableModel* model);

    QListWidgetItem* get_list_item();

    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    QString config_sub_key() const;

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void sgnApplyConfig();
    void sgnDefaultConfig();
    void sgnRestoreConfig();

private:
    QLabel* m_lb_search;
    QLineEdit* m_le_search;
    QTableView* m_table_view_shortcuts;
    QPushButton* m_btn_apply;
    QPushButton* m_btn_default;
    QPushButton* m_btn_restore;
    QHBoxLayout* m_hbl_search_line;
    QHBoxLayout* m_hbl_buttom;
    QVBoxLayout* m_vbl_main;

    QListWidgetItem* m_list_item = nullptr;

    ConfigManager* m_cfg_mgr;
};
