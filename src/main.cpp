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
    // 注意:不设 parent — 所有权唯一归 LoggerManager 的 shared_ptr。
    // 若以 QApplication 为 parent,栈对象 QApplication 先析构时会先删掉本 sink,
    // 随后 LoggerManager(函数内 static,后析构)再次释放 → double free 段错误。
    auto* gui_sink = new LogSinkGui;
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
    const int ret = a.exec();

    // 在 QApplication 存活期内显式释放所有 sink(否则函数内 static 的
    // LoggerManager 会在 QApplication 之后析构,导致 QObject 子类 sink 的
    // 析构发生在 Qt 应用对象销毁之后,存在风险)。
    logger_mgr.clear_sinks();
    return ret;
}
