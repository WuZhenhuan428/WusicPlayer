#pragma once

#include "IConfigBinder.hpp"
#include <QObject>

class LibraryViewBinder : public IConfigBinder
{
public:
    void apply(MainWindowConfigContext& ctx) override;
    void collect(MainWindowConfigContext& ctx) override;
};