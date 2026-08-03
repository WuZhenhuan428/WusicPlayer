#include "app_controller.h"

#include "controller/playback_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/theme/theme_manager.h"
#include "core/theme/builtin/wusic_dark_palette.h"
#include "core/theme/builtin/wusic_light_palette.h"

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
    themeMgr.register_builtin_palette(darkPalette());
    themeMgr.register_builtin_palette(lightPalette());

    Player player;
    PlaybackController playback_controller(&player);
    ConfigManager::get_instance();

    // 在 ConfigManager::load_all() 中恢复上次使用的主题
    ConfigManager::get_instance().register_module(&themeMgr);
    ConfigManager::get_instance().load_all();

    AppController appController(&playback_controller);
    appController.show_main_window();
    return a.exec();
}
