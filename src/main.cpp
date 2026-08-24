#include "app_controller.h"

#include "controller/playback_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/logger/logger_manager.h"
#include "core/theme/theme_manager.h"

#include <QApplication>
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

    // ---- 日志系统初始化 ----
    // 必须先于任何 QObject 子类 sink(LogSinkGui)的构造调用:
    // 静态初始化期创建的文件级 logger 会在此补挂 GUI sink,避免在无 QApplication 时构造 QObject。
    auto& logger_mgr = LoggerManager::instance();
    logger_mgr.mark_app_ready();
    qInstallMessageHandler(LoggerManager::qt_bridge); // Qt 日志转发到统一管道

    QCoreApplication::setApplicationName("WusicPlayer");
    a.setWindowIcon(QIcon(":icons/main.ico"));

    // ---- 主题系统初始化 ----
    // 内建主题(Wusic Dark/Light)现由 AppController 以 BuiltinThemePlugin 注册,
    // 不再使用 register_builtin_palette。
    auto& theme_mgr = ThemeManager::instance();

    Player player;
    PlaybackController playback_controller(&player);
    ConfigManager::get_instance();

    // 在 ConfigManager::load_all() 中恢复上次使用的主题
    ConfigManager::get_instance().register_module(&theme_mgr);
    ConfigManager::get_instance().load_all();

    AppController appController(&playback_controller);
    appController.show_main_window();
    return a.exec();
}
