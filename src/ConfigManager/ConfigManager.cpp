#include "ConfigManager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

QString ConfigManager::getConfigPath() {
    QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    qDebug() << config_path;
    return config_path;
}

void ConfigManager::load() {
    QString folder_path = this->getConfigPath();
    QDir dir(folder_path);
    QString fullpath = dir.filePath("WusicPlayer.json");
    
    if (!dir.exists()) {
        dir.mkdir(folder_path);
    }
    
    QFile file(fullpath);
    if (file.exists()) {
        // read config file
    } else {
        // use default config
    }
}

void ConfigManager::save() {
    // obtain configuration information


    // save as file
    QString folder_path = this->getConfigPath();
    QDir dir(folder_path);
    QString fullpath = dir.filePath("WusicPlayer.json");
    
    if (!dir.exists()) {
        dir.mkdir(folder_path);
    }

    QFile file(fullpath);
    if (file.exists()) {
        // override
    } else {
        // create
    }
}
