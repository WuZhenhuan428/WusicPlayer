#pragma once

#include "core/types.h"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <QMap>
#include <QString>


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

private:    // methods    
    void initUI();
    void initConnections();
    void initTableModel(TrackMetaData meta);

    QString keyToName(const QString& key);

private:    // data structure
    QMap<QString, QStringList> m_meta_buffer;

private:    // ui widgets
    QVBoxLayout* m_vbl_main;

    QHBoxLayout* m_hbl_filepath;
    QLabel* m_lb_filepath;
    QLineEdit* m_le_filepath;

    QTableView* m_table_metadata;
    QHBoxLayout* m_hbl_buttons;
    QPushButton* m_btn_ok;
    QPushButton* m_btn_cancel;
};