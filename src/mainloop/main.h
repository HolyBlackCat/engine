#pragma once

#include "mainloop/module.h"

#include <memory>

namespace em
{
    // Define this in your main file. If this returns null, the main loop doesn't start.
    std::unique_ptr<App::Module> Main();
}
