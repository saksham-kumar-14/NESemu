#include <SDL2/SDL.h>
#include <iostream>
using namespace std;

struct App {
    SDL_Renderer *renderer = nullptr;
    SDL_Window *window = nullptr;
};

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    App app;

    app.window = SDL_CreateWindow(
        "NES Emulator - Stage 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 2, 240 * 2,
        SDL_WINDOW_SHOWN
    );

    if (!app.window) {
        cerr << "SDL Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED);
    if (!app.renderer) {
        cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        SDL_SetRenderDrawColor(app.renderer, 0, 0, 64, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderPresent(app.renderer);
        SDL_Delay(16); // for 60 fps
    }

    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();

    return 0;
}
