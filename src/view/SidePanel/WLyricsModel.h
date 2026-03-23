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

    void setDefaultInfo();
    bool setLocalLrc(const QString& filepath);
    bool setRawLyrics(const QString& raw_data);
    int getRowByPosition(qint64 pos_ms);
    void setCurrentPosition(qint64 pos_ms);
    int currentRow() const;
    QString prevLineText() const;
    QString currentLineText() const;
    QString nextLineText() const;

    
    // QAbstractListModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    
signals:
    void currentLineChanged(const QString& curr_text, const QString& next_text);

private:
    LrcParser m_parser;
    int m_current_row = -1;
};
