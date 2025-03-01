//
// Created by vlad on 2025-03-01.
//

#ifndef TESTSDL_UTILS_H
#define TESTSDL_UTILS_H

#include "SDL3/SDL.h"

struct TouchState {
    float startX;
    float startY;
    bool isSwiping;

    TouchState();  // Constructor declaration
    TouchState(float x, float y);
};


int init(const char *title, int &w, int &h,  SDL_Window **window, SDL_Renderer **renderer);

#endif //TESTSDL_UTILS_H
