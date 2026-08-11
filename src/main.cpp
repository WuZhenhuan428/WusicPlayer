#include "app_controller.h"

#include "controller/playback_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/logger/log_sink_console.h"
#include "core/logger/log_sink_gui.h"
#include "core/logger/logger_manager.h"
#include "core/theme/builtin/wusic_dark_palette.h"
#include "core/theme/builtin/wusic_light_palette.h"
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
    using namespace wusic::log;
    auto& logger_mgr = LoggerManager::instance();
    logger_mgr.add_sink(std::make_shared<ConsoleSink>(true));
    auto* gui_sink = new LogSinkGui(&a); // 生命周期随 QApplication
    logger_mgr.add_sink(std::shared_ptr<LogSink>(gui_sink));
    qInstallMessageHandler(&LoggerManager::qt_bridge); // Qt 日志转发到统一管道

    QCoreApplication::setApplicationName("WusicPlayer");
    a.setWindowIcon(QIcon(":icons/main.ico"));

    // ---- 主题系统初始化 ----
    auto& theme_mgr = ThemeManager::instance();
    theme_mgr.register_builtin_palette(darkPalette());
    theme_mgr.register_builtin_palette(lightPalette());

    Player player;
    PlaybackController playback_controller(&player);
    ConfigManager::get_instance();

    // 在 ConfigManager::load_all() 中恢复上次使用的主题
    ConfigManager::get_instance().register_module(&theme_mgr);
    ConfigManager::get_instance().load_all();

    AppController appController(&playback_controller, gui_sink);
    appController.show_main_window();
    return a.exec();
}
