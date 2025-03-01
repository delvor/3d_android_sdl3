#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <cmath>
#include <SDL3_image/SDL_image.h>

int SCREEN_WIDTH = 600;
int SCREEN_HEIGHT = 800;
const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
const int TILE_SIZE = 64;
const int FOV = 60;
SDL_Texture *wallTexture;

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

struct Player {
    float x, y;
    float angle;
} player = {200, 200, 0};

void update();


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
            mapX = (int) (hitX / TILE_SIZE);
            mapY = (int) (hitY / TILE_SIZE);

            if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT ||
                map[mapY * MAP_WIDTH + mapX] == 1) {
                // Determine if the hit was on a vertical or horizontal wall
                float prevX = player.x + rayX * (distance - 0.1f);
                float prevY = player.y + rayY * (distance - 0.1f);

                int prevMapX = (int)(prevX / TILE_SIZE);
                int prevMapY = (int)(prevY / TILE_SIZE);

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
            textureX = fmod(hitY, TILE_SIZE); // Use Y coordinate for vertical walls
        } else {
            textureX = fmod(hitX, TILE_SIZE); // Use X coordinate for horizontal walls
        }
        textureX = (textureX / TILE_SIZE) * 64; // Scale texture mapping correctly

        // Define source and destination rectangles
        SDL_FRect srcRect = { textureX, 0, 1, 64 }; // 1-pixel slice from the texture
        SDL_FRect destRect = { (float)i, (float)(SCREEN_HEIGHT / 2 - lineHeight / 2), 1, (float)lineHeight };

        SDL_RenderTexture(renderer, wallTexture, &srcRect, &destRect);
    }
}




void render(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    castRays(renderer);
    SDL_RenderPresent(renderer);
}


int main(int argc, [[maybe_unused]] char *argv[]) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done;
    SDL_Event event;


    if (SDL_CreateWindowAndRenderer("SDL3 Raycasting", 0, 0, 0, &window, &renderer) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
        return 2;
    }


    SDL_Log("Attempting to load texture from: %s", "assets/classic_brick_wall_stockcake.jpg");
    wallTexture = loadTexture(renderer, "classic_brick_wall_stockcake.jpg");
    if (!wallTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", SDL_GetError());
        return 1;
    }


    int width, height;
    if (SDL_GetWindowSize(window, &width, &height) == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetCurrentDisplayMode failed: %s",
                     SDL_GetError());
        return 1;
    }

    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;

    SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                done = 1;
        }

        update();
        render(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

void update() {
    player.angle += 0.01f;
}


/*
int main(int argc, [[maybe_unused]] char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("SDL3 Raycasting", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    wallTexture = loadTexture(renderer, "res/drawable/classic_brick_wall_stockcake.jpg");
    if (!wallTexture) return 1;

    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }
        render(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(wallTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
*/