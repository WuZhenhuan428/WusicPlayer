#include "ConfigManager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QJsonObject>
#include <QSaveFile>

namespace {
constexpr int kConfigVersion = 1;
const char* kConfigFileName = "WusicPlayer.json";

QJsonObject columnToJson(const TableColumn& col) {
    QJsonObject obj;
    obj["header"] = col.headerName;
    obj["sort_type"] = static_cast<int>(col.sortType);
    return obj;
}

TableColumn jsonToColumn(const QJsonObject& obj) {
    TableColumn col;
    col.headerName = obj.value("header").toString();
    col.sortType = static_cast<SortType>(obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted)));
    return col;
}
} // namespace

ConfigManager::ConfigManager() {
    setDefault();
    load();
}

QString ConfigManager::getConfigPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString ConfigManager::getConfigFilepath() const {
    QDir dir(getConfigPath());
    return dir.filePath(kConfigFileName);
}

void ConfigManager::setDefault() {
    m_appConfig = AppConfig{};
    m_appConfig.version = kConfigVersion;

    m_appConfig.window.volume = 100;
    m_appConfig.window.isMuted = false;
    
    m_appConfig.playback.position_ms = 0;
    m_appConfig.playback.play_mode = PlayMode::in_order;

    m_appConfig.view.columns = {
        {"", SortType::not_sorted},
        {"Disc", SortType::disc_number},
        {"#", SortType::track_number},
        {"Title", SortType::title},
        {"Artist", SortType::artist},
        {"Duration", SortType::duration},
        {"Album", SortType::album}
    };
}

void ConfigManager::load() {
    setDefault();

    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QFile file(getConfigFilepath());
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[CONFIG] open failed:" << file.fileName() << file.errorString();
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[CONFIG] parse failed:" << err.errorString();
        return;
    }

    const QJsonObject root = doc.object();

    m_appConfig.version = root.value("version").toInt(kConfigVersion);

    const QJsonObject window_obj = root.value("window").toObject();
    m_appConfig.window.geometry = QByteArray::fromBase64(window_obj.value("geometry").toString().toUtf8());
    m_appConfig.window.state = QByteArray::fromBase64(window_obj.value("state").toString().toUtf8());
    m_appConfig.window.volume = window_obj.value("volume").toInt(100);
    m_appConfig.window.isMuted = window_obj.value("is_muted").toBool(false);

    const QJsonObject playback_obj = root.value("playback").toObject();
    const QUuid playlist_id(playback_obj.value("last_playlist_id").toString());
    const QUuid track_id(playback_obj.value("last_track_id").toString());
    m_appConfig.playback.last_playlist_id = playlist_id.isNull() ? QUuid() : playlist_id;
    m_appConfig.playback.last_track_id = track_id.isNull() ? QUuid() : track_id;
    m_appConfig.playback.position_ms = playback_obj.value("position_ms").toInt(0);

    int mode_val = playback_obj.value("play_mode").toInt(static_cast<int>(PlayMode::in_order));
    if (mode_val < static_cast<int>(PlayMode::in_order) || mode_val > static_cast<int>(PlayMode::out_of_order_group)) {
        mode_val = static_cast<int>(PlayMode::in_order);
    }
    m_appConfig.playback.play_mode = static_cast<PlayMode>(mode_val);

    const QJsonObject view_obj = root.value("view").toObject();
    m_appConfig.view.song_tree_header_state = QByteArray::fromBase64(view_obj.value("song_tree_header_state").toString().toUtf8());
    const QJsonArray column_arr = view_obj.value("columns").toArray();
    if (!column_arr.isEmpty()) {
        m_appConfig.view.columns.clear();
        for (const QJsonValue& v : column_arr) {
            if (v.isObject()) {
                m_appConfig.view.columns.append(jsonToColumn(v.toObject()));
            }
        }
    }
}

void ConfigManager::save() {
    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QJsonObject root;
    root["version"] = m_appConfig.version;

    QJsonObject window_obj;
    window_obj["geometry"] = QString::fromUtf8(m_appConfig.window.geometry.toBase64());
    window_obj["state"] = QString::fromUtf8(m_appConfig.window.state.toBase64());
    window_obj["volume"] = m_appConfig.window.volume;
    window_obj["is_muted"] = m_appConfig.window.isMuted;
    root["window"] = window_obj;

    QJsonObject playback_obj;
    playback_obj["last_playlist_id"] = m_appConfig.playback.last_playlist_id.toString(QUuid::WithoutBraces);
    playback_obj["last_track_id"] = m_appConfig.playback.last_track_id.toString(QUuid::WithoutBraces);
    playback_obj["position_ms"] = m_appConfig.playback.position_ms;
    playback_obj["play_mode"] = static_cast<int>(m_appConfig.playback.play_mode);
    root["playback"] = playback_obj;

    QJsonArray column_arr;
    for (const TableColumn& c : m_appConfig.view.columns) {
        column_arr.append(columnToJson(c));
    }
    QJsonObject view_obj;
    view_obj["columns"] = column_arr;
    view_obj["song_tree_header_state"] = QString::fromUtf8(m_appConfig.view.song_tree_header_state.toBase64());
    root["view"] = view_obj;

    QSaveFile file(getConfigFilepath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[CONFIG] write open failed:" << file.fileName() << file.errorString();
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning() << "[CONFIG] commit failed:" << file.fileName() << file.errorString();
    }
}