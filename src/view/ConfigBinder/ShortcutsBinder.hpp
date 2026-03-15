#pragma once

#include "IConfigBinder.hpp"

class ShortcutsBinder : public IConfigBinder
{
public:
    void apply(MainWindowConfigContext& ctx) override;
    void collect(MainWindowConfigContext& ctx) override;
};
