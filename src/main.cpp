#include "app_controller.h"

#include "controller/PlaybackController.h"
#include "core/ConfigManager/ConfigManager.h"
#include "core/theme/ThemeManager.h"
#include "core/theme/builtin/WusicDarkPalette.h"
#include "core/theme/builtin/WusicLightPalette.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <qtenvironmentvariables.h>

int main(int argc, char* argv[])
{
#ifdef Q_OS_LINUX
    // if use wayland. use xcb plugin to enable custom title bar
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QT_IM_MODULE", "fcitx");
#endif

    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("WusicPlayer");
    a.setWindowIcon(QIcon(":icons/main.ico"));

    // ---- 主题系统初始化 ----
    auto& themeMgr = ThemeManager::instance();
    themeMgr.registerBuiltinPalette(darkPalette());
    themeMgr.registerBuiltinPalette(lightPalette());

    Player player;
    PlaybackController playbackController(&player);
    ConfigManager::getInstance();

    // 在 ConfigManager::loadAll() 中恢复上次使用的主题
    ConfigManager::getInstance().registerModule(&themeMgr);
    ConfigManager::getInstance().loadAll();

    AppController appController(&playbackController);
    appController.showMainWindow();
    return a.exec();
}
