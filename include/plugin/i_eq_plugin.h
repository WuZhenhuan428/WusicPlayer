#pragma once

#include "core/player_types.h" // move to inlcude/ later
#include "i_basic_plugin.h"

#include <QtPlugin>

class IEqPlugin : public IBasicPlugin
{
    Q_OBJECT

public:
    virtual QWidget* create_eq_widget(QWidget* parent = nullptr) = 0;
    virtual gains_t gains()                                      = 0; // callback
};

Q_DECLARE_INTERFACE(IEqPlugin, "com.wusicplayer.IEqPlugin/1.0")
