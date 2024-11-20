#pragma once

#include "mainloop/application.h"

#include <memory>

namespace em
{
    // Define this in your main file. If this returns null, the main loop doesn't start.
    std::unique_ptr<AppModule> Main();
}
