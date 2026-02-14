#pragma once

#include <QString>
#include <QVector>
#include <QAbstractListModel>
#include <QVariant>
#include <QPainter>

#include "lrc_parser.h"

class WLyricsModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum UserDefineRole {
        CurrentLine = Qt::UserRole + 1,
        CurrentTime
    };

    explicit WLyricsModel(QObject* parent = nullptr);
    ~WLyricsModel();

    bool setLocalLrc(const QString& filepath);
    void setRawLyrics(const QString& raw_data);
    int getCurrentRow(qint64 pos_ms);
    
    // QAbstractListModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
signals:

private:
    void setStyle();

    LrcParser m_parser;
};

/*
思路：
    1. 初始化创建面板
        文字居中显示
        行居中显示
        隐藏任何列表元素
    2. 切换音乐SINGAL触发加载lrc
        MainWindow::sgnFilepathChanged(QString filepath)
            conect(this, &MainWindow::sgnFilepathChanged, m_lrc_panel, &WLyricsPanel::loadLrc)

        loadLrc:
            do {
                if (m_parser.parseString(raw_data)) {
                    qDebug() << "[LRC] Find lrc in tags";
                    break;
                }
                if (m_parser.parseFile(filepath)) {
                    qDebug() << "[LRC] Find lrc at folder";
                    break;
                } else {
                    qDebug() << "[LRC] Lrc does not exist";
                    break;
                }
            } while(0);

        build view
    3. 信号驱动歌词进度
        接收Player::positionChanged(qint64), 时间刻度在几十毫秒级别
        查找行：二分法查找/计算下一行间隔+QTimer
    4. 其他功能
        双击歌词跳转对应时间
        暂时不实现：ctrl+拖动调整offset，直接拖动调整进度
*/