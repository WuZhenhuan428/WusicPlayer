#pragma once

#include "IConfigBinder.hpp"
#include "MainWindowConfigContext.hpp"

class DesktopLyricsBinder : public IConfigBinder
{
public:
    void apply(MainWindowConfigContext& ctx) override;
    void collect(MainWindowConfigContext& ctx) override;
};