#include "playlist_context.h"
#include <random>

PlaylistContext::PlaylistContext(QObject* parent)
    : QObject(parent)
{
    this->m_mode = PlayMode::in_order;
}

PlaylistContext::~PlaylistContext() {}

void PlaylistContext::setPlayMode(PlayMode mode) {
    if (this->m_mode == mode) { return; }
    this->m_mode = mode;

    emit changedCurrentPlayMode(mode);
}

void PlaylistContext::setPlaylist(const playlistId& pid) {
    if (this->m_currentPlaylistId == pid) { return; }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    m_currentPlaylistId = pid;
    emit changedCurrentListId(pid);
}

void PlaylistContext::setPlayTrack(const trackId& current) {
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    m_currentTrackId = current;
    emit changedCurrentTrackId(current);
}

const playlistId& PlaylistContext::getPlaylistId() {
    return this->m_currentPlaylistId;
}

const trackId& PlaylistContext::getPlayTrackId() {
    return this->m_currentTrackId;
}

PlayMode PlaylistContext::getPlayMode() {
    return this->m_mode;
}


namespace PlaylistNavigator {
    trackId nextOfInOrder(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index != -1 && index < queue.size() - 1) {
            return queue.at(index+1);
        }
        return trackId();
    }

    trackId nextOfLoop(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index != -1)  {
            if (index < queue.size() - 1) {
                return queue.at(index+1);
            }
            else if (index = queue.size() - 1) {
                return queue.at(0);
            }
        }
        return trackId();
    }

    trackId nextOfOutOfOrderTrack(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index != -1 && index <= queue.size()-1) {
            return queue.at(index+1);
        }
        return trackId();
    }

    trackId nextOfShuffle(const QVector<trackId>& queue, trackId current) {
        int index = generate_random_index(queue.size()-1);
        return queue.at(index);
    }

    trackId nextOfOutOfOrderGroup(const QVector<trackId>& queue, trackId current) {
        int index;
        index = queue.indexOf(current);
        if (index != -1 && index <= queue.size()-1) {
            return queue.at(index+1);
        }
        return trackId();
    }


    trackId previousOfInOrder(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index > 0) {    // -1 and 0
            return queue.at(index-1);
        }
        return trackId();
    }

    trackId previousOfLoop(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index != -1)  {
            if (index > 0) {
                return queue.at(index-1);
            }
            else if (index == 0) {
                return queue.at(queue.size()-1);
            }
        }
        return trackId();
    }

    trackId previousOfShuffle(const QVector<trackId>& queue, trackId current) {
        int index = generate_random_index(queue.size()-1);
        return queue.at(index);
    }
    
    trackId previousOfOutOfOrderTrack(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index > 0) {
            return queue.at(index-1);
        }
        return trackId();
    }

    trackId previousOfOutOfOrderGroup(const QVector<trackId>& queue, trackId current) {
        int index = queue.indexOf(current);
        if (index > 0) {
            return queue.at(index-1);
        }
        return trackId();
    }

    size_t generate_random_index(size_t max_index) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, max_index);
        return dist(gen);
    };
}