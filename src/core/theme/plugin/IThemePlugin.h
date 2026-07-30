#pragma once

#include "../ThemePalette.h"
#include <QtPlugin>

/// 外部主题插件接口。
/// 实现此接口的 QObject 子类编译为 .so/.dll，放到 plugins/themes/ 下即可被加载。
class IThemePlugin
{
public:
    virtual ~IThemePlugin()                    = default;

    virtual QString name() const               = 0;
    virtual QString author() const             = 0;
    virtual QString version() const            = 0;
    virtual QString description() const        = 0;

    /// 工厂方法：返回一个填充完整的 ThemePalette
    virtual ThemePalette createPalette() const = 0;
};

Q_DECLARE_INTERFACE(IThemePlugin, "com.wusicplayer.IThemePlugin/1.0")
