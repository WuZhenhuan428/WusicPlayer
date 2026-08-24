// IBasicPlugin 的 moc 编译单元:
// 头文件接口声明了 Q_OBJECT, 必须有 .cpp 让 AUTOMOC 生成 moc
// (staticMetaObject / vtable), 供主程序与外部插件 .so 共同链接。
#include "plugin/i_basic_plugin.h"
