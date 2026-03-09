#include "view/MainWindow.h"

#include <QApplication>
#include "core/ConfigManager/ConfigManager.h"
#include "controller/PlaybackController.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    // if use wayland. use xcb plugin to enable custom title bar
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("WusicPlayer");
    Player player;
    PlaybackController playbackController(&player);
    ConfigManager::getInstance();

    MainWindow w(&playbackController);
    w.show();
    return a.exec();
}
