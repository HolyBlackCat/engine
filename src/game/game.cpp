#include "mainloop/main.h"

#include <iostream>

struct GameApp : em::AppModule
{
    AppAction Init() override
    {
        std::cout << "Hello!\n";
        return AppAction::exit_success;
    }
    AppAction Tick() override {return AppAction::cont;}
    AppAction HandleEvent(SDL_Event &e) override {return AppAction::cont;}
    void Deinit(bool failure) override {(void)failure;}
};

std::unique_ptr<em::AppModule> em::Main()
{
    return std::make_unique<GameApp>();
}
