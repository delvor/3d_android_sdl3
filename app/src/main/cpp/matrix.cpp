#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "utils/utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>
#include <map>
#include <cstring>

// Screen dimensions
int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int COLUMN_WIDTH = 10;
const int CHAR_SIZE = 16;
const int TRAIL_LENGTH_MEAN = 80; // Mean trail length
const int TRAIL_LENGTH_RANGE = 30; // ±20 variation

// Cache for character textures with different alpha levels
struct CharTextureCache {
    std::map<char, std::vector<SDL_Texture *>> textures;
    std::map<char, std::vector<std::pair<int, int>>> dimensions;
    int maxAlphaLevels;

    CharTextureCache(int levels) : maxAlphaLevels(levels) {}

    void init(SDL_Renderer *renderer, TTF_Font *font, const std::string &matrixChars) {
        for (char c: matrixChars) {
            std::vector<SDL_Texture *> &alphaTextures = textures[c];
            std::vector<std::pair<int, int>> &dims = dimensions[c];
            alphaTextures.resize(maxAlphaLevels, nullptr);
            dims.resize(maxAlphaLevels, {0, 0});

            for (int i = 0; i < maxAlphaLevels; i++) {
                int alpha = 255 * (i + 1) / maxAlphaLevels;
                SDL_Color color = {0, static_cast<Uint8>(255 * (i + 1) / maxAlphaLevels), 0,
                                   static_cast<Uint8>(alpha)};
                std::string charStr(1, c);
                SDL_Surface *textSurface = TTF_RenderText_Solid(font, charStr.c_str(),
                                                                strlen(charStr.c_str()), color);
                if (textSurface) {
                    alphaTextures[i] = SDL_CreateTextureFromSurface(renderer, textSurface);
                    dims[i] = {textSurface->w, textSurface->h};
                    SDL_DestroySurface(textSurface);
                }
            }
        }
    }

    SDL_Texture *getTexture(char c, int alphaLevel) {
        auto it = textures.find(c);
        if (it != textures.end() && alphaLevel >= 0 && alphaLevel < maxAlphaLevels) {
            return it->second[alphaLevel];
        }
        return nullptr;
    }

    std::pair<int, int> getDimensions(char c, int alphaLevel) {
        auto it = dimensions.find(c);
        if (it != dimensions.end() && alphaLevel >= 0 && alphaLevel < maxAlphaLevels) {
            return it->second[alphaLevel];
        }
        return {CHAR_SIZE, CHAR_SIZE};
    }

    void cleanup() {
        for (auto &pair: textures) {
            for (SDL_Texture *texture: pair.second) {
                if (texture) SDL_DestroyTexture(texture);
            }
        }
    }
};

struct RainDrop {
    int x;
    std::vector<float> yTrail;
    std::vector<char> chars;
    float speed; // Pixels per second
    int trailIndex;
    int trailLength;

    RainDrop(int startX, float startY, float spd, int maxTrail) : x(startX), speed(spd),
                                                                  trailIndex(0),
                                                                  trailLength(maxTrail) {
        yTrail.resize(trailLength, startY); // Start off-screen, trail builds downward
        chars.resize(trailLength, randomChar());
    }

    static char randomChar() {
        static const std::string matrixChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()";
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(0, (float)matrixChars.size() - 1);
        return matrixChars[dist(gen)];
    }

    void reset(float startY) {
        trailIndex = 0;
        for (int i = 0; i < trailLength; i++) {
            yTrail[i] = startY; // Reset all positions to start off-screen
            chars[i] = randomChar();
        }
    }
};

int runMatrixScreensaver() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    int finish = init("SDL3 Matrix", SCREEN_WIDTH, SCREEN_HEIGHT, &window, &renderer);
    if (finish != 0) return finish;

    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("arial.ttf", CHAR_SIZE);
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
    std::uniform_int_distribution<int> yDist(-CHAR_SIZE * (TRAIL_LENGTH_MEAN + TRAIL_LENGTH_RANGE),
                                             0);
    std::uniform_real_distribution<float> speedDist(100.0f, 400.0f);
    std::uniform_int_distribution<int> trailDist(TRAIL_LENGTH_MEAN - TRAIL_LENGTH_RANGE,
                                                 TRAIL_LENGTH_MEAN + TRAIL_LENGTH_RANGE);

    // Texture cache with max possible trail length
    int maxTrailLength = TRAIL_LENGTH_MEAN + TRAIL_LENGTH_RANGE;
    CharTextureCache charCache(maxTrailLength);
    charCache.init(renderer, font, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()");

    // Initialize rain drops with random trail lengths
    std::vector<RainDrop> drops;
    drops.reserve(SCREEN_WIDTH / COLUMN_WIDTH);
    for (int i = 0; i < SCREEN_WIDTH / COLUMN_WIDTH; i++) {
        float startY = static_cast<float>(yDist(gen));
        int trailLength = trailDist(gen);
        drops.emplace_back(i * COLUMN_WIDTH, startY, speedDist(gen), trailLength);
    }

    bool running = true;
    SDL_Event event;
    Uint64 lastTime = SDL_GetTicks(); // Updated to SDL_GetTicks64
    float deltaTime = 0.0f;

    while (running) {
        Uint64 currentTime = SDL_GetTicks();
        deltaTime = ((float)currentTime - (float)lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                finish = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (auto &drop: drops) {
            if (drop.trailIndex < drop.trailLength) {
                drop.yTrail[drop.trailIndex] += drop.speed * deltaTime;
                drop.chars[drop.trailIndex] = RainDrop::randomChar();
                drop.trailIndex++;
            } else {
                for (int i = 0; i < drop.trailLength - 1; i++) {
                    drop.yTrail[i] = drop.yTrail[i + 1];
                    drop.chars[i] = drop.chars[i + 1];
                }
                drop.yTrail[drop.trailLength - 1] += drop.speed * deltaTime;
                drop.chars[drop.trailLength - 1] = RainDrop::randomChar();
            }

            // Reset drop when it fully exits the screen
            if (drop.yTrail[0] > (float)SCREEN_HEIGHT) { // Check the top of the trail
                drop.reset(static_cast<float>(yDist(
                        gen))); // Restart with a new random off-screen position
            }

            for (int i = 0; i < std::min(drop.trailIndex, drop.trailLength); i++) {
                if (drop.yTrail[i] >= 0) { // Only render characters on-screen
                    int alphaLevel = i;
                    SDL_Texture *textTexture = charCache.getTexture(drop.chars[i], alphaLevel);
                    if (!textTexture) continue;

                    auto [w, h] = charCache.getDimensions(drop.chars[i], alphaLevel);
                    SDL_FRect dstRect = {static_cast<float>(drop.x), drop.yTrail[i],
                                         static_cast<float>(w), static_cast<float>(h)};
                    SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);
                }
            }
        }

        SDL_RenderPresent(renderer);
        // No SDL_Delay; relying on VSync
    }

    charCache.cleanup();
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