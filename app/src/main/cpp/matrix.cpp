//
// Created by vlad on 2025-03-01.
//

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "utils/utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int COLUMN_WIDTH = 10;
const int CHAR_SIZE = 16;


struct RainDrop {
    int x, y, speed;
};

int runMatrixScreensaver() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    int finish = init("SDL3 Matrix", SCREEN_WIDTH,SCREEN_HEIGHT,&window,&renderer);



/*
    if (SDL_CreateWindowAndRenderer("SDL3 Matrix", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window,
                                    &renderer) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
        return 2;
    }

    int width, height;
    if (SDL_GetWindowSize(window, &width, &height) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetCurrentDisplayMode failed: %s",
                     SDL_GetError());
        return 1;
    }*/

    std::srand(std::time(0));

    std::vector<RainDrop> drops;
    for (int i = 0; i < SCREEN_WIDTH / COLUMN_WIDTH; i++) {
        drops.push_back({i * COLUMN_WIDTH, rand() % SCREEN_HEIGHT, 2 + rand() % 5});
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                finish = 0;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw Matrix rain
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (auto &drop: drops) {
            SDL_RenderLine(renderer, drop.x, drop.y, drop.x, drop.y + CHAR_SIZE);
            drop.y += drop.speed;
            if (drop.y > SCREEN_HEIGHT) drop.y = 0; // Reset drop position
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(50);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return finish;
}

int main(int argc, char *argv[]) {
    int val = runMatrixScreensaver();
    return val;
}
