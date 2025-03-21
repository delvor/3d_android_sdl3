//
// Created by vlad on 2025-03-01.
//
#include "utils.h"



// Default constructor
TouchState::TouchState() : startX(0.0f), startY(0.0f), isSwiping(false) {}

// Parameterized constructor
TouchState::TouchState(float x, float y) : startX(x), startY(y), isSwiping(false) {}

int init(const char *title, int &w, int &h,  SDL_Window **window, SDL_Renderer **renderer){
    if (SDL_CreateWindowAndRenderer(title, w, h, SDL_WINDOW_FULLSCREEN, window, renderer) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
        return 2;
    }

    int width, height;
    if (SDL_GetWindowSize(*window, &width, &height) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetCurrentDisplayMode failed: %s",
                     SDL_GetError());
        return 1;
    }

    w = width;
    h = height;

    return  0;
}