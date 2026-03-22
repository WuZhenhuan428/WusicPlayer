#pragma once
#include <QSortFilterProxyModel>

class PlaylistSearchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PlaylistSearchProxyModel(QObject* parent = nullptr);
    ~PlaylistSearchProxyModel() = default;

    void setKeyword(const QString& keyword);
    QString keyword() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool rowMatches(int source_row, const QModelIndex& source_parent) const;
    bool hasAcceptedchildren(int source_row, const QModelIndex source_parent) const;

private:
    QString m_keyword;
    QRegularExpression m_regex;
};