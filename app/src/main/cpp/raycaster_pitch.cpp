#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "quickcg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr int mapWidth = 16;
constexpr int mapHeight = 16;
constexpr int textureSize = 64;
constexpr double pi = 3.14159265358979323846;

struct Sprite
{
    double x;
    double y;
    int texture;
};

struct RayHit
{
    double distance;
    double wallPosition;
    int tile;
    int side;
};

struct TouchPad
{
    SDL_FingerID finger = -1;
    bool active = false;
    float originX = 0.0f;
    float originY = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
};

class RaycasterPitchGame
{
public:
    int run()
    {
        if (!graphics_.open("QuickCG Raycaster Pitch", 640, 360, true)) return 1;
        if (!loadTextures()) return 1;

        width_ = graphics_.width();
        height_ = graphics_.height();
        framebuffer_.resize(static_cast<size_t>(width_) * height_);
        zBuffer_.resize(width_);
        previousTime_ = graphics_.ticks() / 1000.0;

        while (running_)
        {
            processEvents();
            if (!running_) break;

            const Uint64 now = graphics_.ticks();
            const double frameTime = std::min(0.05, std::max(0.001, (now - previousTime_) / 1000.0));
            previousTime_ = now;
            update(frameTime);
            render(frameTime);
        }

        return 0;
    }

private:
    QuickCG::Renderer graphics_;
    const std::array<const char*, mapHeight> map_ = {{
        "1111111111111111",
        "1000000000000001",
        "1000000000000001",
        "1020000000003001",
        "1000000000000001",
        "1000111000000001",
        "1000100000000001",
        "1000105000000001",
        "1000100000000001",
        "1000000000000001",
        "1000004000000001",
        "1000000000000001",
        "1000000000000001",
        "1000000000000001",
        "1000000000000001",
        "1111111111111111"
    }};
    const std::array<Sprite, 4> sprites_ = {{
        {6.5, 2.5, 8}, {12.5, 3.5, 9}, {7.5, 10.5, 8}, {3.5, 12.5, 10}
    }};
    std::array<std::vector<Uint32>, 11> textures_;
    std::vector<Uint32> framebuffer_;
    std::vector<double> zBuffer_;
    TouchPad movePad_;
    TouchPad lookPad_;
    double posX_ = 2.5;
    double posY_ = 2.5;
    double dirX_ = 1.0;
    double dirY_ = 0.0;
    double planeX_ = 0.0;
    double planeY_ = 0.66;
    double pitch_ = 0.0;
    double jump_ = 0.0;
    double previousTime_ = 0.0;
    int width_ = 640;
    int height_ = 360;
    bool running_ = true;
    bool fogEnabled_ = true;

    bool loadTextures()
    {
        const std::array<const char*, 11> names = {{
            "eagle.png", "redbrick.png", "purplestone.png", "greystone.png",
            "bluestone.png", "mossy.png", "wood.png", "colorstone.png",
            "barrel.png", "pillar.png", "greenlight.png"
        }};
        for (size_t i = 0; i < names.size(); i++)
        {
            int textureWidth = 0;
            int textureHeight = 0;
            if (!graphics_.loadImage(names[i], textures_[i], textureWidth, textureHeight) ||
                textureWidth == 0 || textureHeight == 0)
            {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Missing raycaster asset %s: %s", names[i], SDL_GetError());
                return false;
            }
        }
        return true;
    }

    static double clampUnit(double value)
    {
        return std::max(-1.0, std::min(1.0, value));
    }

    int tileAt(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) return 1;
        return map_[y][x] - '0';
    }

    bool blocked(double x, double y) const
    {
        return tileAt(static_cast<int>(x), static_cast<int>(y)) != 0;
    }

    void processEvents()
    {
        SDL_Event event;
        while (graphics_.pollEvent(event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running_ = false;
                break;
            case SDL_EVENT_FINGER_DOWN:
                onFingerDown(event.tfinger);
                break;
            case SDL_EVENT_FINGER_MOTION:
                onFingerMotion(event.tfinger);
                break;
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED:
                onFingerUp(event.tfinger);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) running_ = false;
                if (event.key.key == SDLK_SPACE) jump_ = 48.0;
                if (event.key.key == SDLK_F) fogEnabled_ = !fogEnabled_;
                break;
            default:
                break;
            }
        }
    }

    void onFingerDown(const SDL_TouchFingerEvent& event)
    {
        const float x = event.x * width_;
        const float y = event.y * height_;
        if (x > width_ * 0.76f && y < height_ * 0.20f)
        {
            jump_ = 48.0;
            return;
        }
        if (x > width_ * 0.76f && y < height_ * 0.38f)
        {
            jump_ = -32.0;
            return;
        }

        TouchPad& pad = x < width_ * 0.5f ? movePad_ : lookPad_;
        if (pad.active) return;
        pad.finger = event.fingerID;
        pad.active = true;
        pad.originX = pad.x = x;
        pad.originY = pad.y = y;
    }

    void onFingerMotion(const SDL_TouchFingerEvent& event)
    {
        const float x = event.x * width_;
        const float y = event.y * height_;
        if (movePad_.active && movePad_.finger == event.fingerID)
        {
            movePad_.x = x;
            movePad_.y = y;
        }
        else if (lookPad_.active && lookPad_.finger == event.fingerID)
        {
            lookPad_.x = x;
            lookPad_.y = y;
        }
    }

    void onFingerUp(const SDL_TouchFingerEvent& event)
    {
        if (movePad_.finger == event.fingerID) movePad_ = TouchPad{};
        if (lookPad_.finger == event.fingerID) lookPad_ = TouchPad{};
    }

    void rotate(double angle)
    {
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        const double oldDirX = dirX_;
        const double oldPlaneX = planeX_;
        dirX_ = dirX_ * cosine - dirY_ * sine;
        dirY_ = oldDirX * sine + dirY_ * cosine;
        planeX_ = planeX_ * cosine - planeY_ * sine;
        planeY_ = oldPlaneX * sine + planeY_ * cosine;
    }

    void update(double frameTime)
    {
        const double radius = std::min(width_, height_) * 0.14;
        const double moveX = movePad_.active ? clampUnit((movePad_.x - movePad_.originX) / radius) : 0.0;
        const double moveY = movePad_.active ? clampUnit((movePad_.originY - movePad_.y) / radius) : 0.0;
        const double lookX = lookPad_.active ? clampUnit((lookPad_.x - lookPad_.originX) / radius) : 0.0;
        const double lookY = lookPad_.active ? clampUnit((lookPad_.originY - lookPad_.y) / radius) : 0.0;

        double forward = moveY;
        double strafe = moveX;
        if (graphics_.keyDown(SDLK_W) || graphics_.keyDown(SDLK_UP)) forward += 1.0;
        if (graphics_.keyDown(SDLK_S) || graphics_.keyDown(SDLK_DOWN)) forward -= 1.0;
        if (graphics_.keyDown(SDLK_D)) strafe += 1.0;
        if (graphics_.keyDown(SDLK_A)) strafe -= 1.0;

        const double moveSpeed = frameTime * (graphics_.keyDown(SDLK_LSHIFT) ? 4.0 : 2.6);
        const double strafeX = -dirY_;
        const double strafeY = dirX_;
        const double nextX = posX_ + dirX_ * forward * moveSpeed + strafeX * strafe * moveSpeed;
        const double nextY = posY_ + dirY_ * forward * moveSpeed + strafeY * strafe * moveSpeed;
        if (!blocked(nextX + (nextX > posX_ ? 0.16 : -0.16), posY_)) posX_ = nextX;
        if (!blocked(posX_, nextY + (nextY > posY_ ? 0.16 : -0.16))) posY_ = nextY;

        double turn = lookX;
        if (graphics_.keyDown(SDLK_LEFT)) turn -= 1.0;
        if (graphics_.keyDown(SDLK_RIGHT)) turn += 1.0;
        rotate(turn * frameTime * 1.7);

        double pitchTarget = lookY * 52.0;
        if (graphics_.keyDown(SDLK_PAGEUP)) pitchTarget += 45.0;
        if (graphics_.keyDown(SDLK_PAGEDOWN)) pitchTarget -= 45.0;
        pitch_ += (pitchTarget - pitch_) * std::min(1.0, frameTime * 5.0);
        jump_ += (0.0 - jump_) * std::min(1.0, frameTime * 2.0);
    }

    void renderFloorAndCeiling(double horizon)
    {
        const double leftX = dirX_ - planeX_;
        const double leftY = dirY_ - planeY_;
        const double rightX = dirX_ + planeX_;
        const double rightY = dirY_ + planeY_;
        const double cameraHeight = height_ * 0.5;

        for (int y = 0; y < height_; y++)
        {
            const double rowOffset = std::abs((y + 0.5) - horizon);
            if (rowOffset < 0.75) continue;
            const bool floor = y > horizon;
            const double rowDistance = (cameraHeight + (floor ? jump_ : -jump_)) / rowOffset;
            const double stepX = rowDistance * (rightX - leftX) / width_;
            const double stepY = rowDistance * (rightY - leftY) / width_;
            double worldX = posX_ + rowDistance * leftX;
            double worldY = posY_ + rowDistance * leftY;
            const double shade = std::max(0.2, 1.0 - rowDistance * 0.028);

            for (int x = 0; x < width_; x++)
            {
                const int texX = static_cast<int>((worldX - std::floor(worldX)) * textureSize) & (textureSize - 1);
                const int texY = static_cast<int>((worldY - std::floor(worldY)) * textureSize) & (textureSize - 1);
                const int texture = floor ? (((static_cast<int>(worldX) + static_cast<int>(worldY)) & 1) ? 4 : 3) : 6;
                const Uint32 pixel = textures_[texture][texY * textureSize + texX];
                const double ceilingShade = floor ? 1.0 : 0.72;
                framebuffer_[static_cast<size_t>(y) * width_ + x] = QuickCG::Renderer::shade(pixel, shade * ceilingShade);
                worldX += stepX;
                worldY += stepY;
            }
        }
    }

    RayHit castRay(double rayX, double rayY) const
    {
        int mapX = static_cast<int>(posX_);
        int mapY = static_cast<int>(posY_);
        const double deltaX = rayX == 0.0 ? 1e30 : std::abs(1.0 / rayX);
        const double deltaY = rayY == 0.0 ? 1e30 : std::abs(1.0 / rayY);
        const int stepX = rayX < 0.0 ? -1 : 1;
        const int stepY = rayY < 0.0 ? -1 : 1;
        double sideX = (rayX < 0.0 ? posX_ - mapX : mapX + 1.0 - posX_) * deltaX;
        double sideY = (rayY < 0.0 ? posY_ - mapY : mapY + 1.0 - posY_) * deltaY;
        int side = 0;

        for (int steps = 0; steps < mapWidth * mapHeight; steps++)
        {
            if (sideX < sideY)
            {
                sideX += deltaX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideY += deltaY;
                mapY += stepY;
                side = 1;
            }
            const int tile = tileAt(mapX, mapY);
            if (tile > 0)
            {
                const double distance = std::max(0.001, side == 0 ? sideX - deltaX : sideY - deltaY);
                const double position = side == 0 ? posY_ + distance * rayY : posX_ + distance * rayX;
                return {distance, position - std::floor(position), tile, side};
            }
        }
        return {32.0, 0.0, 1, 0};
    }

    void renderWalls(double horizon)
    {
        for (int x = 0; x < width_; x++)
        {
            const double cameraX = 2.0 * x / width_ - 1.0;
            const double rayX = dirX_ + planeX_ * cameraX;
            const double rayY = dirY_ + planeY_ * cameraX;
            const RayHit hit = castRay(rayX, rayY);
            zBuffer_[x] = hit.distance;

            const int lineHeight = static_cast<int>(height_ / hit.distance);
            const double top = horizon - lineHeight * 0.5 + jump_ / hit.distance;
            const int yStart = std::max(0, static_cast<int>(top));
            const int yEnd = std::min(height_ - 1, static_cast<int>(top + lineHeight));
            int texX = std::min(textureSize - 1, static_cast<int>(hit.wallPosition * textureSize));
            if ((hit.side == 0 && rayX > 0.0) || (hit.side == 1 && rayY < 0.0)) texX = textureSize - texX - 1;
            const int texture = std::min(7, std::max(0, hit.tile - 1));

            for (int y = yStart; y <= yEnd; y++)
            {
                const int texY = std::min(textureSize - 1, std::max(0, static_cast<int>((y - top) * textureSize / lineHeight)));
                Uint32 color = textures_[texture][texY * textureSize + texX];
                if (hit.side == 1) color = QuickCG::Renderer::shade(color, 0.68);
                color = applyFog(color, hit.distance);
                framebuffer_[static_cast<size_t>(y) * width_ + x] = color;
            }
        }
    }

    void renderSprites(double horizon)
    {
        std::array<int, 3> order = {{0, 1, 2}};
        std::sort(order.begin(), order.end(), [&](int a, int b)
        {
            const double ax = sprites_[a].x - posX_;
            const double ay = sprites_[a].y - posY_;
            const double bx = sprites_[b].x - posX_;
            const double by = sprites_[b].y - posY_;
            return ax * ax + ay * ay > bx * bx + by * by;
        });

        for (int index : order)
        {
            const Sprite& sprite = sprites_[index];
            const double relativeX = sprite.x - posX_;
            const double relativeY = sprite.y - posY_;
            const double determinant = planeX_ * dirY_ - dirX_ * planeY_;
            const double transformX = (dirY_ * relativeX - dirX_ * relativeY) / determinant;
            const double transformY = (-planeY_ * relativeX + planeX_ * relativeY) / determinant;
            if (transformY <= 0.1) continue;

            const int screenX = static_cast<int>(width_ * 0.5 * (1.0 + transformX / transformY));
            const int size = std::min(height_ * 3, std::abs(static_cast<int>(height_ / transformY)));
            const int startX = std::max(0, screenX - size / 2);
            const int endX = std::min(width_, screenX + size / 2);
            const int startY = std::max(0, static_cast<int>(horizon - size * 0.5 + jump_ / transformY));
            const int endY = std::min(height_, static_cast<int>(horizon + size * 0.5 + jump_ / transformY));

            for (int stripe = startX; stripe < endX; stripe++)
            {
                if (transformY >= zBuffer_[stripe]) continue;
                const int texX = std::min(textureSize - 1, (stripe - (screenX - size / 2)) * textureSize / std::max(size, 1));
                for (int y = startY; y < endY; y++)
                {
                    const int texY = std::min(textureSize - 1, (y - startY) * textureSize / std::max(endY - startY, 1));
                    const Uint32 color = textures_[sprite.texture][texY * textureSize + texX];
                    if (color == 0) continue;
                    framebuffer_[static_cast<size_t>(y) * width_ + stripe] = applyFog(color, transformY);
                }
            }
        }
    }

    Uint32 applyFog(Uint32 color, double distance) const
    {
        const double fog = std::min(0.82, std::max(0.0, (distance - 4.0) / 22.0));
        const int red = static_cast<int>(((color >> 16) & 255) * (1.0 - fog) + 18 * fog);
        const int green = static_cast<int>(((color >> 8) & 255) * (1.0 - fog) + 29 * fog);
        const int blue = static_cast<int>((color & 255) * (1.0 - fog) + 42 * fog);
        return (red << 16) | (green << 8) | blue;
    }

    void renderTouchControls()
    {
        const int radius = std::min(width_, height_) / 9;
        const int leftX = movePad_.active ? static_cast<int>(movePad_.originX) : width_ / 5;
        const int leftY = movePad_.active ? static_cast<int>(movePad_.originY) : height_ * 4 / 5;
        const int rightX = lookPad_.active ? static_cast<int>(lookPad_.originX) : width_ * 4 / 5;
        const int rightY = lookPad_.active ? static_cast<int>(lookPad_.originY) : height_ * 4 / 5;
        graphics_.drawCircle(leftX, leftY, radius, 0xDDE8F0, false);
        graphics_.drawCircle(rightX, rightY, radius, 0xDDE8F0, false);
        if (movePad_.active)
            graphics_.drawCircle(static_cast<int>(movePad_.x), static_cast<int>(movePad_.y), radius / 3, 0x55C6A7, true);
        if (lookPad_.active)
            graphics_.drawCircle(static_cast<int>(lookPad_.x), static_cast<int>(lookPad_.y), radius / 3, 0xF5C45E, true);
    }

    void render(double frameTime)
    {
        const double horizon = height_ * 0.5 + pitch_;
        std::fill(framebuffer_.begin(), framebuffer_.end(), 0x101820);
        renderFloorAndCeiling(horizon);
        renderWalls(horizon);
        renderSprites(horizon);

        graphics_.drawBufferRGB(framebuffer_.data(), framebuffer_.size());
        graphics_.drawText("LEFT: MOVE / STRAFE     RIGHT: TURN / LOOK", 14, 14, {255, 255, 255, 255});
        graphics_.drawText("TAP TOP-RIGHT: JUMP / CROUCH", 14, 38, {255, 220, 130, 255});
        graphics_.drawText("FPS " + std::to_string(static_cast<int>(1.0 / frameTime)), 14, 62, {130, 230, 180, 255});
        renderTouchControls();
        graphics_.present();
    }
};
}

int main(int, char**)
{
    RaycasterPitchGame game;
    return game.run();
}
