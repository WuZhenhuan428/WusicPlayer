#pragma once

#include "core/types.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

class TagEditWidget : public QWidget
{
    Q_OBJECT

public:
    /**  TrackMetaData只包含播放器需要的项目，作为tag编辑工具需要解析所有的元数据
     *  meta: 主要使用filepath
     *  tid: 用于写回时定位
     */
    explicit TagEditWidget(TrackMetaData meta, EntryId tid, QWidget* parent = nullptr);
    ~TagEditWidget();

private: // methods
    void init_ui();
    void init_ui_properties(const QString& filepath);
    void init_connections();
    void init_table_model(TrackMetaData meta);

    void handle_save_tags();
    void handle_show_menu(const QPoint& pos);

    QString key_to_name(const QString& key);
    QString name_to_key(const QString& name);

    void handle_edit_item(QModelIndex index);
    void handle_delete_item(QModelIndex index);
    void handle_add_new_field();

signals:
    void sgnSaveTags(QMap<QString, QStringList> tags, EntryId tid);

private: // data structure
    QStandardItemModel* m_table_model = nullptr;
    QMap<QString, QStringList> m_meta_buffer;
    EntryId m_tid;

private: // ui widgets
    QVBoxLayout* m_vbl_main;

    QHBoxLayout* m_hbl_filepath;
    QLabel* m_lb_filepath;
    QLineEdit* m_le_filepath;

    QTableView* m_table_metadata;
    QHBoxLayout* m_hbl_buttons;
    QPushButton* m_btn_help;
    QPushButton* m_btn_ok;
    QPushButton* m_btn_cancel;
};
