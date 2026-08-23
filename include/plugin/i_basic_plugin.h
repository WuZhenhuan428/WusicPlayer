#pragma once

#include <QString>
#include <QVector>
#include <QtPlugin>

class IBasicPlugin : public QObject
{
    Q_OBJECT

public:
    ~IBasicPlugin() override                    = default;

    // 元数据提供接口: 优先使用外部 json 进行描述, 当外部 json 不存在时,
    // 再使用内部接口进行解析 (例如添加内置插件)

    /// id 格式要求: <域名>.<作者/组织>.<插件名称>/<GNU风格版本号>, 某项为空时保留小数点,
    /// 在实现时保持 id 与 iid 一致, 版本号后的 build 时间可以为空
    virtual QString id() const                  = 0;
    virtual QString name() const                = 0;
    virtual QString version() const             = 0;
    virtual QString description() const         = 0;
    virtual QString author() const              = 0;
    virtual QVector<QString> categories() const = 0;
};

Q_DECLARE_INTERFACE(IBasicPlugin, /*IId*/ "com.wusicplayer.IBasicPlugin/1.0.0")
