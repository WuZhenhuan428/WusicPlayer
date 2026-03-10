#pragma once

#include "view/ConfigBinder/MainWindowConfigContext.hpp"

class IConfigBinder
{
public:
    virtual void apply(MainWindowConfigContext& ctx) = 0;
    virtual void collect(MainWindowConfigContext& ctx) = 0;
};

