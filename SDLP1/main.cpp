#include <SDL.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define WIDTH 800
#define HEIGHT 600

constexpr int MAX_ITER = 1000;

using Precision = double; // TODO(deep-zoom): swap to multiprecision or perturbation reference coordinates.

constexpr Precision X_MIN = -2.5;
constexpr Precision X_MAX = 1.0;
constexpr Precision Y_MIN = -1.0;
constexpr Precision Y_MAX = 1.0;

constexpr Precision ZOOM_FACTOR_IN = 1.1;
constexpr Precision ZOOM_FACTOR_OUT = 1.0 / ZOOM_FACTOR_IN;
constexpr Precision PAN_STEP = 0.1;
constexpr Precision DOUBLE_PRECISION_LIMIT_SCALE = 1e12; // Beyond this, adjacent pixels collapse in double.

struct ViewState {
    Precision zoom = 1.0;
    Precision offsetX = 0.0;
    Precision offsetY = 0.0;

    Precision minX() const { return X_MIN / zoom + offsetX; }
    Precision maxX() const { return X_MAX / zoom + offsetX; }
    Precision minY() const { return Y_MIN / zoom + offsetY; }
    Precision maxY() const { return Y_MAX / zoom + offsetY; }
    Precision scale() const { return 1.0 / zoom; }
};

Precision mapValue(Precision value, Precision inMin, Precision inMax, Precision outMin, Precision outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

SDL_Color hueToRGB(double hue) {
    const double h = hue / 60.0;
    const int i = static_cast<int>(h);
    const double f = h - i;
    const double p = 0.0;
    const double q = 255 * (1.0 - f);
    const double t = 255 * f;

    switch (i % 6) {
    case 0:
        return {255, static_cast<Uint8>(t), static_cast<Uint8>(p), 255};
    case 1:
        return {static_cast<Uint8>(q), 255, static_cast<Uint8>(p), 255};
    case 2:
        return {static_cast<Uint8>(p), 255, static_cast<Uint8>(t), 255};
    case 3:
        return {static_cast<Uint8>(p), static_cast<Uint8>(q), 255, 255};
    case 4:
        return {static_cast<Uint8>(t), static_cast<Uint8>(p), 255, 255};
    default:
        return {255, static_cast<Uint8>(p), static_cast<Uint8>(q), 255};
    }
}

int mandelbrot(Precision x0, Precision y0) {
    Precision x = 0.0;
    Precision y = 0.0;
    int iter = 0;
    while (x * x + y * y < 4.0 && iter < MAX_ITER) {
        const Precision xTemp = x * x - y * y + x0;
        y = 2.0 * x * y + y0;
        x = xTemp;
        ++iter;
    }
    return iter;
}

Precision screenToComplexX(int x, const ViewState& view) {
    return mapValue(static_cast<Precision>(x), 0.0, static_cast<Precision>(WIDTH), view.minX(), view.maxX());
}

Precision screenToComplexY(int y, const ViewState& view) {
    return mapValue(static_cast<Precision>(y), 0.0, static_cast<Precision>(HEIGHT), view.minY(), view.maxY());
}

void applyCursorZoom(int mouseX, int mouseY, Precision zoomFactor, ViewState& view) {
    const Precision beforeX = screenToComplexX(mouseX, view);
    const Precision beforeY = screenToComplexY(mouseY, view);

    view.zoom *= zoomFactor;

    const Precision mappedX = screenToComplexX(mouseX, view);
    const Precision mappedY = screenToComplexY(mouseY, view);

    view.offsetX += beforeX - mappedX;
    view.offsetY += beforeY - mappedY;
}

bool saveScreenshot(SDL_Renderer* renderer, const std::string& filename) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return false;
    }

    const int readPixelsResult = SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch);
    if (readPixelsResult != 0) {
        SDL_FreeSurface(surface);
        return false;
    }

    const int saveResult = SDL_SaveBMP(surface, filename.c_str());
    SDL_FreeSurface(surface);
    return saveResult == 0;
}

std::string buildWindowTitle(const ViewState& view, bool precisionWarning) {
    std::ostringstream title;
    title << std::fixed << std::setprecision(12)
          << "Mandelbrot | center=(" << view.offsetX << ", " << view.offsetY << ")"
          << " scale=" << view.scale();

    if (precisionWarning) {
        title << " | WARNING: Double precision unreliable at this zoom";
    }

    return title.str();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Mandelbrot Set", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (!win) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    ViewState view;
    int screenshotIndex = 1;
    bool screenshotPending = false;

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_r:
                    view = ViewState{};
                    break;
                case SDLK_s:
                    screenshotPending = true;
                    break;
                case SDLK_LEFT:
                    view.offsetX -= PAN_STEP / view.zoom;
                    break;
                case SDLK_RIGHT:
                    view.offsetX += PAN_STEP / view.zoom;
                    break;
                case SDLK_UP:
                    view.offsetY -= PAN_STEP / view.zoom;
                    break;
                case SDLK_DOWN:
                    view.offsetY += PAN_STEP / view.zoom;
                    break;
                default:
                    break;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                int mouseX = 0;
                int mouseY = 0;
                SDL_GetMouseState(&mouseX, &mouseY);
                if (event.wheel.y > 0) {
                    applyCursorZoom(mouseX, mouseY, ZOOM_FACTOR_IN, view);
                } else if (event.wheel.y < 0) {
                    applyCursorZoom(mouseX, mouseY, ZOOM_FACTOR_OUT, view);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                const int x = event.button.x;
                const int y = event.button.y;

                if (event.button.button == SDL_BUTTON_LEFT) {
                    applyCursorZoom(x, y, ZOOM_FACTOR_IN, view);
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    applyCursorZoom(x, y, ZOOM_FACTOR_OUT, view);
                }
            }
        }

        const bool precisionWarning = view.zoom > DOUBLE_PRECISION_LIMIT_SCALE;
        SDL_SetWindowTitle(win, buildWindowTitle(view, precisionWarning).c_str());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                const Precision x0 = screenToComplexX(x, view);
                const Precision y0 = screenToComplexY(y, view);

                const int color = mandelbrot(x0, y0);
                const double hue = std::fmod(color * 10.0, 360.0);
                const SDL_Color rgb = hueToRGB(hue);
                SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }

        if (screenshotPending) {
            const std::string filename = "screenshot_" + std::to_string(screenshotIndex++) + ".bmp";
            if (!saveScreenshot(renderer, filename)) {
                std::cout << "Failed to save screenshot: " << SDL_GetError() << std::endl;
            }
            screenshotPending = false;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
