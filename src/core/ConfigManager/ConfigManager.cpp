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
    setWindowGeometry(
        QByteArray::fromBase64(window_obj.value("geometry").toString().toUtf8())
    );
    setWindowState(
        QByteArray::fromBase64(window_obj.value("state").toString().toUtf8())
    );
    setVolume(window_obj.value("volume").toInt(100));
    setMute(window_obj.value("is_muted").toBool(false));

    const QJsonObject playback_obj = root.value("playback").toObject();
    const playlistId pid(playback_obj.value("last_pid").toString());
    const trackId tid(playback_obj.value("last_tid").toString());
    setLastPlayInfo(
        (pid.isNull() ? playlistId() : pid),
        (tid.isNull() ? trackId() : tid),
        (playback_obj.value("position_ms").toInt(0))
    );

    int mode_val = playback_obj.value("play_mode").toInt(static_cast<int>(PlayMode::in_order));
    if (mode_val < static_cast<int>(PlayMode::in_order) || mode_val > static_cast<int>(PlayMode::out_of_order_group)) {
        mode_val = static_cast<int>(PlayMode::in_order);
    }
    setPlayMode(static_cast<PlayMode>(mode_val));

    const QJsonObject view_obj = root.value("view").toObject();
    setSongTreeViewHeader(
        QByteArray::fromBase64(view_obj.value("state").toString().toUtf8())
    );
    const QJsonArray column_arr = view_obj.value("columns").toArray();
    if (!column_arr.isEmpty()) {
        m_appConfig.view.columns.clear();
        for (const QJsonValue& v : column_arr) {
            if (v.isObject()) {
                m_appConfig.view.columns.append(jsonToColumn(v.toObject()));
            }
        }
    }

    const QJsonObject search_panel_obj = root.value("search_panel").toObject();
    setSearchPanelGeometry(
        QByteArray::fromBase64(search_panel_obj.value("geometry").toString().toUtf8())
    );
    setSearchPanelHeader(
        QByteArray::fromBase64(search_panel_obj.value("state").toString().toUtf8())
    );
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
    playback_obj["last_pid"] = m_appConfig.playback.last_pid.toString(QUuid::WithoutBraces);
    playback_obj["last_tid"] = m_appConfig.playback.last_tid.toString(QUuid::WithoutBraces);
    playback_obj["position_ms"] = m_appConfig.playback.position_ms;
    playback_obj["play_mode"] = static_cast<int>(m_appConfig.playback.play_mode);
    root["playback"] = playback_obj;

    QJsonArray column_arr;
    for (const TableColumn& c : m_appConfig.view.columns) {
        column_arr.append(columnToJson(c));
    }
    QJsonObject view_obj;
    view_obj["columns"] = column_arr;
    view_obj["state"] = QString::fromUtf8(m_appConfig.view.state.toBase64());
    root["view"] = view_obj;

    QJsonObject search_panel_obj;
    search_panel_obj["geometry"] = QString::fromUtf8(m_appConfig.search_panel.geometry.toBase64());
    search_panel_obj["state"] = QString::fromUtf8(m_appConfig.search_panel.state.toBase64());
    root["search_panel"] = search_panel_obj;

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

const AppConfig& ConfigManager::getAppConfig() const {
    return m_appConfig;
}

const AppConfig::WindowState& ConfigManager::getWindowState() const {
    return m_appConfig.window;
}

const AppConfig::PlaybackState& ConfigManager::getPlaybackState() const {
    return m_appConfig.playback;
}

const AppConfig::ViewSettings& ConfigManager::getViewState() const {
    return m_appConfig.view;
}

const AppConfig::SearchPanel& ConfigManager::getSearchPanelState() const {
    return m_appConfig.search_panel;
}

void ConfigManager::setWindowGeometry(const QByteArray& geo) {
    m_appConfig.window.geometry = geo;
}

void ConfigManager::setWindowState(const QByteArray& state) {
    m_appConfig.window.state = state;
}

void ConfigManager::setVolume(int volume) {
    m_appConfig.window.volume = volume;
}

void ConfigManager::setMute(bool is_mute) {
    m_appConfig.window.isMuted= is_mute;
}

void ConfigManager::setLastPlayInfo(const playlistId& pid, const trackId& tid, int position_ms) {
    m_appConfig.playback.last_pid = pid;
    m_appConfig.playback.last_tid = tid;
    m_appConfig.playback.position_ms = position_ms;
}

void ConfigManager::setPlayMode(PlayMode mode) {
    m_appConfig.playback.play_mode = mode;
}

void ConfigManager::setTableColumns(const QList<TableColumn>& columns) {
    m_appConfig.view.columns = columns;
}

void ConfigManager::setSongTreeViewHeader(QByteArray header) {
    m_appConfig.view.state = header;
}

void ConfigManager::setSearchPanelGeometry(QByteArray geo) {
    m_appConfig.search_panel.geometry = geo;
}

void ConfigManager::setSearchPanelHeader(QByteArray header) {
    m_appConfig.search_panel.state = header;
}
