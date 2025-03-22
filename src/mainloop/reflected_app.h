#pragma once

#include "em/macros/utils/forward.h"
#include "em/refl/for_each_matching_elem.h"
#include "mainloop/module.h"

namespace em::App
{
    // This is not a base class. Wrap your most derived class in this.
    // This doesn't inherit from `T` because then there seems to be no good way to handle the case where the base class doesn't override
    //   one of the functions, which causes us to recurse infinitely. There are two ways around that kinda looked promising:
    // 1. We could call the functions below using `m.decltype(m)::Tick()` (or whatever), but that breaks polymorphism, i.e. if someone
    //   wants to store a module using a base class pointer.
    // 2. We could check if the function is overridden in `T` via `if constexpr (std::is_same_v<decltype(&T::func), decltype(&Module::func)>)`,
    //   but that breaks down if the user starts adding overloads of `func` (we could also test for inability to take the address,
    //   but then we don't know if that should result in true or false).
    template <typename T>
    struct ReflectedApp : Module
    {
        T underlying;

        ReflectedApp(auto &&... params) : underlying(EM_FWD(params)...) {}

        Action Tick() override
        {
            // `exit_success` is non-zero (zero is `cont`). This causes us to exit if there are no overriders,
            //   to avoid an infinite loop, which apparently is only stoppable by a SIGKILL.
            Action ret = Action::exit_success;
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopAnyOf<>>(underlying, [&](Module &m){return bool(ret = m.Tick());});
            return ret;
        }

        Action HandleEvent(SDL_Event &e) override
        {
            Action ret = Action::cont;
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopAnyOf<>>(underlying, [&](Module &m){return bool(ret = m.HandleEvent(e));});
            return ret;
        }
    };
}
