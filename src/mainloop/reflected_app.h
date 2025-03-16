#pragma once

#include "em/refl/for_each_matching_elem.h"
#include "mainloop/module.h"

namespace em::App
{
    // This is not a base class. Wrap your most derived class in this.
    template <typename Base>
    struct ReflectedApp : Base
    {
        using Base::Base;

        Action Init() override
        {
            Action ret = Action::cont;
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopAnyOf<>, Refl::IterationFlags::ignore_root>(static_cast<Base &>(*this), [&](Module &m){return bool(ret = m.Init());});
            return ret;
        }

        Action Tick() override
        {
            Action ret = Action::cont;
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopAnyOf<>, Refl::IterationFlags::ignore_root>(static_cast<Base &>(*this), [&](Module &m){return bool(ret = m.Tick());});
            return ret;
        }

        Action HandleEvent(SDL_Event &e) override
        {
            Action ret = Action::cont;
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopAnyOf<>, Refl::IterationFlags::ignore_root>(static_cast<Base &>(*this), [&](Module &m){return bool(ret = m.HandleEvent(e));});
            return ret;
        }

        void Deinit(bool failure) override
        {
            Refl::ForEachElemOfTypeCvref<Module, Meta::LoopSimple_Reverse, Refl::IterationFlags::ignore_root>(static_cast<Base &>(*this), [&](Module &m){return m.Deinit(failure);});
        }
    };
}
