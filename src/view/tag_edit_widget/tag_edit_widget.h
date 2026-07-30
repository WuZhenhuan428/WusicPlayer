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
    explicit TagEditWidget(TrackMetaData meta, trackId tid, QWidget* parent = nullptr);
    ~TagEditWidget();

private: // methods
    void initUI();
    void initUIProperties(const QString& filepath);
    void initConnections();
    void initTableModel(TrackMetaData meta);

    void handleSaveTags();
    void handleShowMenu(const QPoint& pos);

    QString keyToName(const QString& key);
    QString nameToKey(const QString& name);

    void handleEditItem(QModelIndex index);
    void handleDeleteItem(QModelIndex index);
    void handleAddNewFiled();

signals:
    void sgnSaveTags(QMap<QString, QStringList> tags, trackId tid);

private: // data structure
    QStandardItemModel* m_table_model = nullptr;
    QMap<QString, QStringList> m_meta_buffer;
    trackId m_tid;

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
