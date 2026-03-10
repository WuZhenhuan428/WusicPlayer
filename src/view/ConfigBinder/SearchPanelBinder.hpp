#pragma once

#include "IConfigBinder.hpp"

class SearchPanelBinder : public IConfigBinder
{
public:
    void apply(MainWindowConfigContext& ctx) override;
    void collect(MainWindowConfigContext& ctx) override;
};