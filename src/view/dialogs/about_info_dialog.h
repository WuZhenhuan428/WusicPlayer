#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

/// 仿照 KDE 系列软件的关于页面(竖版)进行设计

class AboutItem : public QWidget
{
public:
    AboutItem(const QString& title, const QString& text, const QString& url = QString{},
              QWidget* parent = nullptr);

private:
    QVBoxLayout* vbl_text_ = nullptr;
    QLabel* lb_title_      = nullptr;
    QLabel* lb_text_       = nullptr;
    QPushButton* btn_url_  = nullptr;
    QHBoxLayout* hbl_main_ = nullptr;

    QString url_;
};

class LicenseDialog : public QWidget
{};

class AboutInfoDialog : public QWidget
{
public:
    AboutInfoDialog(QWidget* parent = nullptr);

private:
private:
    // 1. 头部
    QHBoxLayout* hbl_header_;

    QVBoxLayout* vbl_text_;
    QLabel* lb_app_icon_;
    QLabel* lb_app_name_;
    QLabel* lb_license_;
    QLabel* lb_app_version_;

    // 2. 标签页与子项
    QTabWidget* tab_body_;

    // 2.1. 关于
    QWidget* wrap_about_;
    QVBoxLayout* vbl_about_;
    QLabel* lb_app_desc_;
    QLabel* lb_copyright_;
    QLabel* lb_repo_page_;

    // 2.2. 模块
    QWidget* wrap_modules_;
    QVBoxLayout* vbl_modules_;
    AboutItem* it_module_qt_;
    AboutItem* it_module_ffmpeg_;
    AboutItem* it_module_miniaudio_;
    AboutItem* it_module_taglib_;
    AboutItem* it_module_magic_enum_;

    // 2.3. 作者
    QWidget* wrap_authors_;
    QVBoxLayout* vbl_author_;
    AboutItem* it_author_wzh_;

    // 3. 底部按键
    QHBoxLayout* hbl_buttons_;
    QPushButton* btn_close_;

    QVBoxLayout* vbl_main_;
};
