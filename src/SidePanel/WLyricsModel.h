#pragma once

#include <QString>
#include <QVector>
#include <QAbstractListModel>
#include <QVariant>
#include <QPainter>

#include "lrc_parser.h"
#include "../../src/core/types.h"

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

    void setDefaultInfo();
    bool setLocalLrc(const QString& filepath);
    bool setRawLyrics(const QString& raw_data);
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
