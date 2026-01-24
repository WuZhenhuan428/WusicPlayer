#pragma once

#include <QObject>
#include <QString>
#include <QUuid>

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"

class PlaylistManager : public QObject
{
    Q_OBJECT

    PlaylistContext* m_context = new PlaylistContext;
    PlaylistRepo* m_repo = new PlaylistRepo;
    PlaylistViewModel* m_view = new PlaylistViewModel(m_repo);

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();

public:
    PlaylistViewModel* getViewModel();
    const QUuid& getCurrentPlaylist() const;

public slots:
    // receive signals from UI
    void createPlaylist();
    void removePlaylist();
    void copyPlaylist();
    void loadPlaylist(const QString& playlist_path);
    void renamePlaylist();
    void saveCurrentPlaylist(const QString& save_path);

    void addTrack(const QString& filepath);

    void play(int index);

signals:
    void requestPlay(const QString& filepath);

private:

};