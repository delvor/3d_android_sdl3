#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <cmath>
#include <SDL3_image/SDL_image.h>
#include "objects/player.h"
#include "utils/utils.h"


int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const int TEXTURE_SIZE = 64;
const int FOV = 60;

const float PLAYER_MOVE_RATE = 2.0f;
const float PLAYER_TURN_RATE = 0.1f;

SDL_Texture *wallTexture;
SDL_Texture *floorTexture;
SDL_Texture *ceilingTexture;

TouchState touch;


int map[MAP_WIDTH * MAP_HEIGHT] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 1, 0, 0, 0, 0, 1,
        1, 0, 0, 0, 1, 0, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1
};

Player player = {200, 200, 0};


SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filename);
void update();
void render(SDL_Renderer *renderer);
void drawFloorAndCeiling(SDL_Renderer *pRenderer);
void castRays(SDL_Renderer *renderer);
void handleTouch(SDL_Event &event);


int main(int argc, [[maybe_unused]] char *argv[]) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done;
    SDL_Event event;
    int finish = init("SDL3 Raycasting Maze", SCREEN_WIDTH, SCREEN_HEIGHT, &window, &renderer);


    SDL_Log("Attempting to load texture from: %s", "bricksx64.png");
    wallTexture = loadTexture(renderer, "bricksx64.png");
    if (!wallTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("Attempting to load texture from: %s", "Gravel64.png");
    floorTexture = loadTexture(renderer, "Gravel64.png");
    if (!floorTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("Attempting to load texture from: %s", "WalkStone64.png");
    ceilingTexture = loadTexture(renderer, "WalkStone64.png");
    if (!ceilingTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", SDL_GetError());
        return 1;
    }

    SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                done = 1;
            else if (event.type == SDL_EVENT_FINGER_DOWN ||
                     event.type == SDL_EVENT_FINGER_MOTION ||
                     event.type == SDL_EVENT_FINGER_UP) {
                handleTouch(event);
            }
        }

        update();
        render(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return finish;
}


SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filename) {
    SDL_Surface *surface = IMG_Load(filename);
    if (!surface) {
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}


void castRays(SDL_Renderer *renderer) {
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        float rayAngle = (player.angle - (FOV / 2.0f) * ((float) M_PI / 180.0f)) +
                         ((float) i / (float) SCREEN_WIDTH) * (FOV * ((float) M_PI / 180.0f));
        float rayX = cos(rayAngle);
        float rayY = sin(rayAngle);

        float distance = 0.1f;
        float hitX, hitY;
        int mapX, mapY;
        bool hitVerticalWall = false;

        while (distance < 1000) {
            hitX = player.x + rayX * distance;
            hitY = player.y + rayY * distance;
            mapX = (int) (hitX / TEXTURE_SIZE);
            mapY = (int) (hitY / TEXTURE_SIZE);

            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                map[mapY * MAP_WIDTH + mapX] == 1) {
                // Determine if the hit was on a vertical or horizontal wall
                float prevX = player.x + rayX * (distance - 0.1f);
                float prevY = player.y + rayY * (distance - 0.1f);

                int prevMapX = (int) (prevX / TEXTURE_SIZE);
                int prevMapY = (int) (prevY / TEXTURE_SIZE);

                // If X coordinate changed more, it's a vertical wall (left/right)
                if (prevMapX != mapX) {
                    hitVerticalWall = true;
                }
                break;
            }
            distance += 0.1f;
        }

        // Correct fish-eye distortion
        float correctedDist = distance * cos(rayAngle - player.angle);
        int lineHeight = SCREEN_HEIGHT / (correctedDist * 0.02f);

        // Get the correct texture X coordinate
        float textureX;
        if (hitVerticalWall) {
            textureX = fmod(hitY, TEXTURE_SIZE); // Use Y coordinate for vertical walls
        } else {
            textureX = fmod(hitX, TEXTURE_SIZE); // Use X coordinate for horizontal walls
        }
        textureX = (textureX / TEXTURE_SIZE) * 64; // Scale texture mapping correctly

        // Define source and destination rectangles
        SDL_FRect srcRect = {textureX, 0, 1, 64}; // 1-pixel slice from the texture
        SDL_FRect destRect = {(float) i, (float) (SCREEN_HEIGHT / 2 - lineHeight / 2), 1,
                              (float) lineHeight};

        SDL_RenderTexture(renderer, wallTexture, &srcRect, &destRect);
    }
}

void render(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    drawFloorAndCeiling(renderer);
    castRays(renderer);
    SDL_RenderPresent(renderer);
}


void drawFloorAndCeiling(SDL_Renderer *renderer) {
    for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++) {
        float rowDistance = (0.5f * SCREEN_HEIGHT) / (y - SCREEN_HEIGHT / 2);

        float floorStepX = rowDistance * cos(player.angle);
        float floorStepY = rowDistance * sin(player.angle);

        float floorX = player.x + floorStepX;
        float floorY = player.y + floorStepY;

        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int tileX = (int)(floorX / TEXTURE_SIZE);
            int tileY = (int)(floorY / TEXTURE_SIZE);

            if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT) {
                floorX += floorStepX / SCREEN_WIDTH;
                floorY += floorStepY / SCREEN_WIDTH;
                continue;
            }

            int texX = (int)(floorX) % TEXTURE_SIZE;
            int texY = (int)(floorY) % TEXTURE_SIZE;

            SDL_FRect srcRect = { (float)texX, (float)texY, 1, 1 };
            SDL_FRect destRect = { (float)x, (float)y, 1, 1 };

            SDL_RenderTexture(renderer, floorTexture, &srcRect, &destRect);

            destRect.y = SCREEN_HEIGHT - y;
            SDL_RenderTexture(renderer, ceilingTexture, &srcRect, &destRect);

            floorX += floorStepX / SCREEN_WIDTH;
            floorY += floorStepY / SCREEN_WIDTH;
        }
    }
}



void handleTouch(SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_FINGER_DOWN:
            touch.startX = event.tfinger.x;
            touch.startY = event.tfinger.y;
            touch.isSwiping = true;
            break;

        case SDL_EVENT_FINGER_MOTION:
            if (touch.isSwiping) {
                float endX = event.tfinger.x;
                float endY = event.tfinger.y;
                float deltaX = endX - touch.startX;
                float deltaY = endY - touch.startY;

                if (fabs(deltaY) > fabs(deltaX)) { // Vertical swipe

                    player.x -= cos(player.angle) * deltaY * PLAYER_MOVE_RATE;
                    player.y -= sin(player.angle) * deltaY * PLAYER_MOVE_RATE;

                } else { // Horizontal swipe

                    player.angle += deltaX * PLAYER_TURN_RATE;

                }
            }
            break;
        case SDL_EVENT_FINGER_UP:

            touch.isSwiping = false;
            break;

    }
}


void update() {
    //player.angle += 0.05f;
}


