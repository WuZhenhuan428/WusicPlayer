#pragma once

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class LibraryManager;

/**
 * @brief 设置面板 "Media Library" 页:媒体库根目录(watched folders)管理。
 *
 * 这是添加媒体库路径的唯一入口。依赖注入 LibraryManager(非拥有,可空)。
 */
class LibrarySettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit LibrarySettingsPage(LibraryManager* lib = nullptr, QWidget* parent = nullptr);

    QListWidgetItem* getTitleItem();

private:
    void initUI();
    void refreshFolders();
    void addFolder();
    void removeSelected();

    LibraryManager* m_lib         = nullptr; // 非拥有
    QListWidget* m_list           = nullptr;
    QPushButton* m_btn_add        = nullptr;
    QPushButton* m_btn_remove     = nullptr;
    QListWidgetItem* m_title_item = nullptr;
};
