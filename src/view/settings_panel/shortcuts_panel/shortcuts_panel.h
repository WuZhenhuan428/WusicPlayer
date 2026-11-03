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

    void set_view_model(QAbstractTableModel* model);

    QListWidgetItem* get_list_item();

    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    [[nodiscard]] static QString config_sub_key();

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void sgn_apply_config();
    void sgn_default_config();
    void sgn_restore_config();

private:
    QLabel* lb_search_;
    QLineEdit* le_search_;
    QTableView* table_view_shortcuts_;
    QPushButton* btn_apply_;
    QPushButton* btn_default_;
    QPushButton* btn_restore_;
    QHBoxLayout* hbl_search_line_;
    QHBoxLayout* hbl_buttom_;
    QVBoxLayout* vbl_main_;

    QListWidgetItem* list_item_ = nullptr;

    ConfigManager* cfg_mgr_;
};
