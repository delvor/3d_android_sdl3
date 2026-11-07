#ifndef QUICKCG_H
#define QUICKCG_H

#include <SDL3/SDL.h>

#include <cstddef>
#include <string>
#include <vector>

struct TTF_Font;

namespace QuickCG
{

class Renderer
{
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool open(const std::string& title, int width, int height, bool fullscreen = true);
    void close();
    bool pollEvent(SDL_Event& event);
    bool keyDown(SDL_Keycode key) const;
    Uint64 ticks() const;
    int width() const;
    int height() const;

    bool loadImage(const std::string& path, std::vector<Uint32>& pixels,
                   int& imageWidth, int& imageHeight) const;
    void clear(Uint32 rgb = 0);
    static Uint32 shade(Uint32 rgb, double amount);
    void setPixel(int x, int y, Uint32 rgb);
    void drawLine(int x0, int y0, int x1, int y1, Uint32 rgb);
    void drawCircle(int centerX, int centerY, int radius, Uint32 rgb, bool filled = false);
    void drawBufferRGB(const Uint32* pixels, std::size_t count);
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void present();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* frameTexture_ = nullptr;
    ::TTF_Font* font_ = nullptr;
    std::vector<Uint32> frame_;
    std::vector<Uint8> rgba_;
    int width_ = 0;
    int height_ = 0;
    bool ownsVideoInit_ = false;
    bool ownsTtfInit_ = false;
};

} // namespace QuickCG

#endif
