#pragma once

#include "core/types.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LibraryManager;
class QComboBox;

/**
 * @brief 设置面板 "Media Library" 页:媒体库根目录(watched folders)管理
 *        + 添加未入库文件的解析策略配置。
 *
 * 这是添加媒体库路径的唯一入口。依赖注入 LibraryManager(非拥有,可空)。
 */
class LibrarySettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit LibrarySettingsPage(LibraryManager* lib = nullptr, QWidget* parent = nullptr);

    QListWidgetItem* get_title_item();

    // 同步下拉框到当前策略(不触发 sgnAddFilePolicyChanged)
    void set_add_file_policy(AddFilePolicy policy);

signals:
    // 用户切换策略;参数为 AddFilePolicy 的 int 值
    void sgnAddFilePolicyChanged(int policy);

private:
    void init_ui();
    void refresh_folders();
    void add_folder();
    void remove_selected();

    LibraryManager* m_lib         = nullptr; // 非拥有
    QListWidget* m_list           = nullptr;
    QComboBox* m_cb_add_policy    = nullptr;
    QPushButton* m_btn_add        = nullptr;
    QPushButton* m_btn_remove     = nullptr;
    QListWidgetItem* m_title_item = nullptr;
};
