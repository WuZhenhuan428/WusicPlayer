#include "MainWindow.h"

#include <QApplication>
#include "src/ConfigManager/ConfigManager.h"
#include "src/controller/PlaybackController.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("WusicPlayer");
    Player player;
    PlaybackController playbackController(&player);
    ConfigManager::getInstance();

    MainWindow w(&playbackController);
    w.show();
    return a.exec();
}
