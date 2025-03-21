#include "SDL3/SDL.h"
#include <SDL3/SDL_main.h>
#include "SDL3_ttf/SDL_ttf.h" // Add SDL_ttf for text rendering
#include "utils/utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>

// Screen dimensions
int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int COLUMN_WIDTH = 10; // Width between columns
const int CHAR_SIZE = 16;    // Size of each character
const int TRAIL_LENGTH = 20; // Number of characters in trail

struct RainDrop {
    int x;                     // Column x-position
    std::vector<int> yTrail;   // Trail positions (y-coordinates)
    std::vector<char> chars;   // Characters in the trail
    int speed;                 // Speed of drop
    int trailIndex;            // Current index in trail

    RainDrop(int startX, int startY, int spd) : x(startX), speed(spd), trailIndex(0) {
        // Initialize trail with starting position
        yTrail.resize(TRAIL_LENGTH, startY);
        chars.resize(TRAIL_LENGTH, randomChar());
    }

    static char randomChar() {
        // Randomly select a Matrix-like character (e.g., letters, numbers, symbols)
        const std::string matrixChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()";
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(0, matrixChars.size() - 1);
        return matrixChars[dist(gen)];
    }
};

int runMatrixScreensaver() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    // Initialize SDL and SDL_ttf
    int finish = init("SDL3 Matrix", SCREEN_WIDTH, SCREEN_HEIGHT, &window, &renderer);
    if (finish != 0) return finish;

    if (!TTF_Init() ) {
        SDL_Log("TTF_Init failed: %s",  SDL_GetError());
        return 1;
    }

    // Load a font (you'll need a TrueType font file, e.g., "arial.ttf")
    TTF_Font *font = TTF_OpenFont("arial.ttf", CHAR_SIZE); // Replace with your font path
    if (!font) {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> yDist(0, SCREEN_HEIGHT);
    std::uniform_int_distribution<int> speedDist(6, 26);

    // Initialize rain drops
    std::vector<RainDrop> drops;
    drops.reserve(SCREEN_WIDTH / COLUMN_WIDTH);
    for (int i = 0; i < SCREEN_WIDTH / COLUMN_WIDTH; i++) {
        int startY = yDist(gen);
        drops.emplace_back(i * COLUMN_WIDTH, startY, speedDist(gen));
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

        // Update and draw Matrix rain
        for (auto &drop : drops) {
            // Update trail positions
            if (drop.trailIndex < TRAIL_LENGTH) {
                drop.yTrail[drop.trailIndex] = drop.yTrail[drop.trailIndex] + drop.speed;
                drop.chars[drop.trailIndex] = RainDrop::randomChar();
            } else {
                // Shift trail up and add new character at the bottom
                for (int i = 0; i < TRAIL_LENGTH - 1; i++) {
                    drop.yTrail[i] = drop.yTrail[i + 1];
                    drop.chars[i] = drop.chars[i + 1];
                }
                drop.yTrail[TRAIL_LENGTH - 1] += drop.speed;
                drop.chars[TRAIL_LENGTH - 1] = RainDrop::randomChar();
            }

            // Reset if the head of the trail goes off-screen
            if (drop.yTrail[TRAIL_LENGTH - 1] > SCREEN_HEIGHT) {
                for (int i = 0; i < TRAIL_LENGTH; i++) {
                    drop.yTrail[i] = -CHAR_SIZE * (TRAIL_LENGTH - i);
                    drop.chars[i] = RainDrop::randomChar();
                }
                drop.trailIndex = 0;
            } else {
                drop.trailIndex++;
            }

            // Draw the trail with fading effect
            for (int i = 0; i < std::min(drop.trailIndex, TRAIL_LENGTH); i++) {
                int alpha = 255 * (i + 1) / TRAIL_LENGTH; // Fade effect
                SDL_Color color = {0, static_cast<Uint8>(255 * (i + 1) / TRAIL_LENGTH), 0, static_cast<Uint8>(alpha)};

                // Render the character
                std::string charStr(1, drop.chars[i]);
                SDL_Surface *textSurface = TTF_RenderText_Solid(font, charStr.c_str(), strlen(charStr.c_str()), color);
                if (!textSurface) continue;

                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (!textTexture) {
                    SDL_DestroySurface(textSurface);
                    continue;
                }

                SDL_FRect dstRect = {(float)drop.x, (float)drop.yTrail[i], (float)textSurface->w, (float)textSurface->h};
                SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);

                SDL_DestroyTexture(textTexture);
                SDL_DestroySurface(textSurface);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return finish;
}

int main(int argc, char *argv[]) {
    return runMatrixScreensaver();
}