#include <QApplication>
#include "core/ConfigManager/ConfigManager.h"
#include "controller/PlaybackController.h"
#include "app_controller.h"
#include <QDebug>
#include <qtenvironmentvariables.h>

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    // if use wayland. use xcb plugin to enable custom title bar
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QT_IM_MODULE", "fcitx");
#endif

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("WusicPlayer");
    Player player;
    PlaybackController playbackController(&player);
    ConfigManager::getInstance();

    AppController appController(&playbackController);
    appController.showMainWindow();
    return a.exec();
}
