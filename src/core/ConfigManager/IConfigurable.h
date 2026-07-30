/**
 * USAGE:
 * 在需要读写配置的组件中继承本接口类并实现对应接口，然后在顶层模块中的ConfigManager示例中注册
 * NOTICE: 不要在此处继承QObject以及存储任何数据，防止组件多继承时出现错误
 */
#pragma once
class QJsonObject;
class QString;

class IConfigurable
{
public:
    virtual ~IConfigurable()                           = default;
    virtual void loadFromJson(const QJsonObject& json) = 0;
    virtual QJsonObject saveToJson()                   = 0;
    virtual QString configSubKey() const               = 0;
};
