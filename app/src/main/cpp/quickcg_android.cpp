#include "quickcg.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace QuickCG
{

Renderer::Renderer() = default;

Renderer::~Renderer()
{
    close();
}

bool Renderer::open(const std::string& title, int width, int height, bool fullscreen)
{
    close();

    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0)
    {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL video init failed: %s", SDL_GetError());
            return false;
        }
        ownsVideoInit_ = true;
    }

    if (!TTF_WasInit())
    {
        if (TTF_Init()) ownsTtfInit_ = true;
        else SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_ttf init failed: %s", SDL_GetError());
    }

    const SDL_WindowFlags flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    window_ = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (!window_)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL window creation failed: %s", SDL_GetError());
        close();
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL renderer creation failed: %s", SDL_GetError());
        close();
        return false;
    }

    width_ = width;
    height_ = height;
    frame_.assign(static_cast<size_t>(width_) * height_, 0);
    rgba_.resize(frame_.size() * 4);

    frameTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                      SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (!frameTexture_)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL frame texture creation failed: %s", SDL_GetError());
        close();
        return false;
    }
    SDL_SetTextureBlendMode(frameTexture_, SDL_BLENDMODE_NONE);

    font_ = TTF_OpenFont("arial.ttf", 16.0f);
    if (!font_) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open arial.ttf asset: %s", SDL_GetError());
    return true;
}

void Renderer::close()
{
    if (font_)
    {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    if (frameTexture_)
    {
        SDL_DestroyTexture(frameTexture_);
        frameTexture_ = nullptr;
    }
    if (renderer_)
    {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    frame_.clear();
    rgba_.clear();
    width_ = 0;
    height_ = 0;

    if (ownsTtfInit_)
    {
        TTF_Quit();
        ownsTtfInit_ = false;
    }
    if (ownsVideoInit_)
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        ownsVideoInit_ = false;
    }
}

bool Renderer::pollEvent(SDL_Event& event)
{
    return SDL_PollEvent(&event);
}

bool Renderer::keyDown(SDL_Keycode key) const
{
    const SDL_Scancode code = SDL_GetScancodeFromKey(key, nullptr);
    int keyCount = 0;
    const bool* state = SDL_GetKeyboardState(&keyCount);
    return state && code > SDL_SCANCODE_UNKNOWN && code < keyCount && state[code];
}

Uint64 Renderer::ticks() const
{
    return SDL_GetTicks();
}

int Renderer::width() const
{
    return width_;
}

int Renderer::height() const
{
    return height_;
}

bool Renderer::loadImage(const std::string& path, std::vector<Uint32>& pixels,
                         int& imageWidth, int& imageHeight) const
{
    SDL_Surface* loaded = IMG_Load(path.c_str());
    if (!loaded)
    {
        const size_t separator = path.find_last_of("/\\");
        if (separator != std::string::npos)
            loaded = IMG_Load(path.substr(separator + 1).c_str());
    }
    if (!loaded) return false;

    SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!rgba) return false;

    imageWidth = rgba->w;
    imageHeight = rgba->h;
    pixels.resize(static_cast<size_t>(imageWidth) * imageHeight);
    const Uint8* bytes = static_cast<const Uint8*>(rgba->pixels);
    for (int y = 0; y < imageHeight; y++)
    {
        const Uint8* row = bytes + static_cast<size_t>(y) * rgba->pitch;
        for (int x = 0; x < imageWidth; x++)
        {
            const Uint8* pixel = row + x * 4;
            pixels[static_cast<size_t>(y) * imageWidth + x] = pixel[3] == 0
                ? 0
                : (static_cast<Uint32>(pixel[0]) << 16) |
                  (static_cast<Uint32>(pixel[1]) << 8) | pixel[2];
        }
    }
    SDL_DestroySurface(rgba);
    return true;
}

void Renderer::clear(Uint32 rgb)
{
    std::fill(frame_.begin(), frame_.end(), rgb & 0x00FFFFFFu);
}

Uint32 Renderer::shade(Uint32 rgb, double amount)
{
    const double factor = std::max(0.0, std::min(1.0, amount));
    const Uint32 red = static_cast<Uint32>(((rgb >> 16) & 255) * factor);
    const Uint32 green = static_cast<Uint32>(((rgb >> 8) & 255) * factor);
    const Uint32 blue = static_cast<Uint32>((rgb & 255) * factor);
    return (red << 16) | (green << 8) | blue;
}

void Renderer::setPixel(int x, int y, Uint32 rgb)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    frame_[static_cast<size_t>(y) * width_ + x] = rgb & 0x00FFFFFFu;
}

void Renderer::drawLine(int x0, int y0, int x1, int y1, Uint32 rgb)
{
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;)
    {
        setPixel(x0, y0, rgb);
        if (x0 == x1 && y0 == y1) break;
        const int doubledError = 2 * error;
        if (doubledError >= dy)
        {
            error += dy;
            x0 += sx;
        }
        if (doubledError <= dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

void Renderer::drawCircle(int centerX, int centerY, int radius, Uint32 rgb, bool filled)
{
    if (radius <= 0) return;
    const int radiusSquared = radius * radius;
    for (int y = -radius; y <= radius; y++)
    {
        const int halfWidth = static_cast<int>(std::sqrt(radiusSquared - y * y));
        if (filled) drawLine(centerX - halfWidth, centerY + y, centerX + halfWidth, centerY + y, rgb);
        else
        {
            setPixel(centerX - halfWidth, centerY + y, rgb);
            setPixel(centerX + halfWidth, centerY + y, rgb);
        }
    }
}

void Renderer::drawBufferRGB(const Uint32* pixels, size_t count)
{
    if (!pixels || count < frame_.size()) return;
    std::copy(pixels, pixels + frame_.size(), frame_.begin());
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color)
{
    if (!font_ || text.empty()) return;
    SDL_Surface* surface = TTF_RenderText_Solid(font_, text.c_str(), text.size(), color);
    if (!surface) return;
    SDL_Surface* rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!rgba) return;

    const Uint8* bytes = static_cast<const Uint8*>(rgba->pixels);
    for (int row = 0; row < rgba->h; row++)
    {
        for (int column = 0; column < rgba->w; column++)
        {
            const int dstX = x + column;
            const int dstY = y + row;
            if (dstX < 0 || dstY < 0 || dstX >= width_ || dstY >= height_) continue;
            const Uint8* source = bytes + static_cast<size_t>(row) * rgba->pitch + column * 4;
            const unsigned int alpha = source[3];
            if (alpha == 0) continue;
            Uint32& destination = frame_[static_cast<size_t>(dstY) * width_ + dstX];
            const unsigned int inverse = 255 - alpha;
            const unsigned int red = ((destination >> 16) & 255) * inverse + source[0] * alpha;
            const unsigned int green = ((destination >> 8) & 255) * inverse + source[1] * alpha;
            const unsigned int blue = (destination & 255) * inverse + source[2] * alpha;
            destination = ((red / 255) << 16) | ((green / 255) << 8) | (blue / 255);
        }
    }
    SDL_DestroySurface(rgba);
}

void Renderer::present()
{
    if (!renderer_ || !frameTexture_) return;
    for (size_t i = 0; i < frame_.size(); i++)
    {
        const Uint32 pixel = frame_[i];
        rgba_[i * 4] = static_cast<Uint8>(pixel >> 16);
        rgba_[i * 4 + 1] = static_cast<Uint8>(pixel >> 8);
        rgba_[i * 4 + 2] = static_cast<Uint8>(pixel);
        rgba_[i * 4 + 3] = 255;
    }
    SDL_UpdateTexture(frameTexture_, nullptr, rgba_.data(), width_ * 4);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, frameTexture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

} // namespace QuickCG
