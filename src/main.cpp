#include "app_controller.h"

#include "controller/playback_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/config_manager/language_settings.h"
#include "core/logger/logger_manager.h"
#include "core/theme/theme_manager.h"

#include <QApplication>
#include <QIcon>
#include <QTranslator>
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
    ConfigManager& cfg_mgr  = ConfigManager::get_instance();

    // 多语言: 语言设置模块在 load_all() 前注册, 以便恢复上次选择的语言
    auto& language_settings = LanguageSettings::instance();
    cfg_mgr.register_module(&language_settings);

    // 在 ConfigManager::load_all() 中恢复上次使用的主题/语言
    cfg_mgr.register_module(&theme_mgr);
    cfg_mgr.load_all();

    // 按语言加载翻译(.qm 位于可执行文件旁的 translations/)
    QTranslator translator;
    const QString qm_path = QCoreApplication::applicationDirPath() +
                            QStringLiteral("/translations/wusicplayer_") +
                            language_settings.locale() + QStringLiteral(".qm");
    Logger* i18n_logger = LoggerManager::file_logger("i18n", {"console", "gui"});
    if (translator.load(qm_path)) {
        a.installTranslator(&translator);
        i18n_logger->info("translation loaded: {}", qm_path.toStdString());
    } else {
        i18n_logger->warn("translation NOT found: {}", qm_path.toStdString());
    }

    AppController appController(&playback_controller);
    appController.show_main_window();
    return a.exec();
}
