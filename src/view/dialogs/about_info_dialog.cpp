#include "about_info_dialog.h"

#include "core/utils/font.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QStyle>
#include <QUrl>

#include "core/logger/logger_manager.h"
namespace
{
static auto logger = LoggerManager::instance().file_logger("about_dialog", {"console", "gui"});
}

AboutItem::AboutItem(const QString& title, const QString& text, const QString& url,
                     QWidget* parent) : QWidget(parent), url_(url)
{
    // 获取默认字体及字号
    QFont title_font = utils::font::get_relative_size_font_default(4);

    // 初始化 UI
    vbl_text_        = new QVBoxLayout;
    lb_title_        = new QLabel(title, this);
    lb_title_->setFont(title_font);
    lb_text_ = new QLabel(text, this);
    vbl_text_->addWidget(lb_title_);
    vbl_text_->addWidget(lb_text_);

    hbl_main_ = new QHBoxLayout;
    hbl_main_->addLayout(vbl_text_);

    if (!url.isEmpty()) {
        if (!btn_url_) {
            btn_url_ = new QPushButton(this);
            btn_url_->setFixedSize(25, 25);
            btn_url_->setIcon(this->style()->standardIcon(QStyle::SP_CommandLink));
            hbl_main_->addWidget(btn_url_);
            btn_url_->setToolTip(QString("%2").arg(url));
            connect(btn_url_, &QPushButton::clicked, this, [this]() {
                logger->debug("about item: {}", url_);
                QDesktopServices::openUrl(QUrl(url_));
            });
        }
    }

    this->setLayout(hbl_main_);
}

AboutInfoDialog::AboutInfoDialog(QWidget* parent) : QWidget(parent)
{
    this->setWindowTitle("About");
    this->setWindowFlag(Qt::Window);

    // 1.1 头部文字
    hbl_header_  = new QHBoxLayout;
    vbl_text_    = new QVBoxLayout;
    lb_app_name_ = new QLabel("WusicPlayer", this);
    lb_app_name_->setFont(utils::font::get_relative_size_font_default(8));
    lb_app_version_ = new QLabel(tr("Version: ") + "0.0.1", this);
    vbl_text_->addWidget(lb_app_name_);
    vbl_text_->addWidget(lb_app_version_);

    // 1.2 加载图标
    lb_app_icon_ = new QLabel(this);
    auto icon    = QPixmap(":icons/main_64_64.ico");
    lb_app_icon_->setPixmap(std::move(icon));
    hbl_header_->setAlignment(Qt::AlignLeft);
    hbl_header_->addWidget(lb_app_icon_);
    hbl_header_->addLayout(vbl_text_);

    // 2. 标签页与子项
    tab_body_   = new QTabWidget(this);

    // 2.1. 关于, 设置打开外部链接
    wrap_about_ = new QWidget(this);
    vbl_about_  = new QVBoxLayout;
    lb_app_desc_ =
        new QLabel(tr("WusicPlayer, a modern, multifunctional local music player"), wrap_about_);
    lb_copyright_ = new QLabel("© 2025 - 2026, " + tr("The WusicPlayer Authors"), wrap_about_);

    lb_license_   = new QLabel(wrap_about_);
    lb_license_->setTextFormat(Qt::RichText);
    lb_license_->setText(
        QString("%1: <a href='https://www.gnu.org/licenses/gpl-3.0.en.html'>%2</a>")
            .arg(tr("License"))
            .arg(tr("GNU Generral Public License Version 3")));
    lb_license_->setOpenExternalLinks(false);
    connect(lb_license_, &QLabel::linkActivated, this, [](const QString& str) {
        logger->info("open license page");
        QDesktopServices::openUrl(QUrl(str));
    });

    lb_repo_page_ = new QLabel(wrap_about_);
    lb_repo_page_->setTextFormat(Qt::RichText);
    lb_repo_page_->setText(QString("<a href='https://github.com/wuZhenhuan428/WusicPlayer/'>%1</a>")
                               .arg(tr("Github Page")));
    connect(lb_repo_page_, &QLabel::linkActivated, this, [](const QString& str) {
        logger->info("open github page: {}", str);
        QDesktopServices::openUrl(QUrl(str));
    });

    vbl_about_->addWidget(lb_app_desc_);
    vbl_about_->addWidget(lb_copyright_);
    vbl_about_->addWidget(lb_license_);
    vbl_about_->addWidget(lb_repo_page_);
    vbl_about_->addStretch();
    wrap_about_->setLayout(vbl_about_);

    // 2.2. 模块
    wrap_modules_ = new QWidget(this);
    vbl_modules_  = new QVBoxLayout();
    it_module_qt_ = new AboutItem("Qt 6", tr("A cross-platform application development framework."),
                                  "https://www.qt.io", wrap_modules_);
    it_module_ffmpeg_ = new AboutItem(
        "FFmpeg",
        tr("A complete, cross-platform solution to record, convert and stream audio and video."),
        "https://ffmpeg.org", wrap_modules_);
    it_module_miniaudio_ =
        new AboutItem("miniaudio", tr("An audio playback and capture library for C and C++."),
                      "miniaud.io", wrap_modules_);
    it_module_taglib_ = new AboutItem("TagLib",
                                      tr("TagLib is a library for reading and editing the metadata "
                                         "of several popular audio formats."),
                                      "https://taglib.org", wrap_modules_);
    it_module_magic_enum_ =
        new AboutItem("magic_enum",
                      tr("Header-only C++17 library provides static reflection for enums, work "
                         "with any enum type without any macro or boilerplate code."),
                      "https://github.com/Neargye/magic_enum", wrap_modules_);
    vbl_modules_->addWidget(it_module_qt_);
    vbl_modules_->addWidget(it_module_ffmpeg_);
    vbl_modules_->addWidget(it_module_miniaudio_);
    vbl_modules_->addWidget(it_module_taglib_);
    vbl_modules_->addWidget(it_module_magic_enum_);
    vbl_modules_->addStretch();
    wrap_modules_->setLayout(vbl_modules_);

    // 2.3. 作者
    wrap_authors_  = new QWidget(this);
    vbl_author_    = new QVBoxLayout;
    it_author_wzh_ = new AboutItem(
        "Zhenhuan Wu (吴震寰)", tr("the current main author") + ", email: wuzhenhuan545@gmail.com",
        QString{}, wrap_authors_);
    vbl_author_->addWidget(it_author_wzh_);
    vbl_author_->addStretch();
    wrap_authors_->setLayout(vbl_author_);

    // 2.4 组装标签页
    tab_body_->addTab(wrap_about_, tr("About"));
    tab_body_->addTab(wrap_modules_, tr("Modules"));
    tab_body_->addTab(wrap_authors_, tr("Authors"));

    // 3. 关闭按键
    btn_close_ = new QPushButton(tr("Close"), this);
    btn_close_->setIcon(this->style()->standardIcon(QStyle::SP_DialogCloseButton));
    hbl_buttons_ = new QHBoxLayout;
    hbl_buttons_->addStretch();
    hbl_buttons_->addWidget(btn_close_);

    // 4. 组装主布局
    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_header_);
    vbl_main_->addWidget(tab_body_);
    vbl_main_->addLayout(hbl_buttons_);

    this->setLayout(vbl_main_);

    connect(btn_close_, &QPushButton::clicked, this, &QWidget::close);
}
