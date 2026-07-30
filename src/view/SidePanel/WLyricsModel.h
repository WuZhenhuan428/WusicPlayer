#pragma once

#include "core/types.h"
#include "lrc_parser.h"

#include <QAbstractListModel>
#include <QPainter>
#include <QString>
#include <QVariant>
#include <QVector>

class WLyricsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum UserDefineRole
    {
        CurrentLine = Qt::UserRole + 1,
        CurrentTime
    };

    explicit WLyricsModel(QObject* parent = nullptr);
    ~WLyricsModel();

    void setDefaultInfo(const TrackMetaData& meta);
    bool setLocalLrc(const QString& filepath);
    bool setLrcFilePath(const QString& lrc_path);
    bool setRawLyrics(const QString& raw_data);
    int getRowByPosition(qint64 pos_ms);
    QString rawLyricsText() const;
    void setCurrentPosition(qint64 pos_ms);
    int currentRow() const;
    QString prevLineText() const;
    QString currentLineText() const;
    QString nextLineText() const;

    // QAbstractListModel interface
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

signals:
    void currentLineChanged(const QString& curr_text, const QString& next_text);
    void sgnUseTimelineFollow(bool enable);

private:
    LrcParser m_parser;
    LrcParser::LrcFile m_lrc_file;
    QString m_raw_lyrics_text;
    int m_current_row = -1;
};
