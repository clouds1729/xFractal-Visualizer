#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define WIDTH 800
#define HEIGHT 600

constexpr double X_MIN = -2.5;
constexpr double X_MAX = 1.0;
constexpr double Y_MIN = -1.0;
constexpr double Y_MAX = 1.0;

constexpr double ZOOM_FACTOR_IN = 1.12;
constexpr double ZOOM_FACTOR_OUT = 1.0 / ZOOM_FACTOR_IN;
constexpr double PAN_STEP = 0.08;

struct ViewState {
    double zoom = 1.0;
    double centerX = (X_MIN + X_MAX) * 0.5;
    double centerY = (Y_MIN + Y_MAX) * 0.5;
};

enum class PaletteMode { Classic = 0, Fire = 1, Ice = 2 };

static double mapValue(double value, double inMin, double inMax, double outMin, double outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

static double viewportWidth(const ViewState& view) { return (X_MAX - X_MIN) / view.zoom; }
static double viewportHeight(const ViewState& view) { return (Y_MAX - Y_MIN) / view.zoom; }

static double screenToComplexX(int x, const ViewState& view) {
    const double halfW = viewportWidth(view) * 0.5;
    return mapValue(static_cast<double>(x), 0.0, static_cast<double>(WIDTH), view.centerX - halfW, view.centerX + halfW);
}

static double screenToComplexY(int y, const ViewState& view) {
    const double halfH = viewportHeight(view) * 0.5;
    return mapValue(static_cast<double>(y), 0.0, static_cast<double>(HEIGHT), view.centerY - halfH, view.centerY + halfH);
}

static void applyCursorZoom(int mouseX, int mouseY, double factor, ViewState& view) {
    const double beforeX = screenToComplexX(mouseX, view);
    const double beforeY = screenToComplexY(mouseY, view);

    view.zoom *= factor;

    const double afterX = screenToComplexX(mouseX, view);
    const double afterY = screenToComplexY(mouseY, view);

    view.centerX += beforeX - afterX;
    view.centerY += beforeY - afterY;
}

static int adaptiveIterations(double zoom, int manualOffset) {
    const double logZoom = std::max(0.0, std::log10(zoom));
    const int base = 120 + static_cast<int>(logZoom * 90.0);
    return std::clamp(base + manualOffset, 50, 4000);
}

static SDL_Color paletteColor(double t, PaletteMode palette) {
    t = std::clamp(t, 0.0, 1.0);
    if (palette == PaletteMode::Classic) {
        const Uint8 r = static_cast<Uint8>(9 * (1 - t) * t * t * t * 255);
        const Uint8 g = static_cast<Uint8>(15 * (1 - t) * (1 - t) * t * t * 255);
        const Uint8 b = static_cast<Uint8>(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
        return {r, g, b, 255};
    }
    if (palette == PaletteMode::Fire) {
        return {static_cast<Uint8>(255 * t), static_cast<Uint8>(180 * t * t), static_cast<Uint8>(80 * (1 - t)), 255};
    }
    return {static_cast<Uint8>(100 * (1 - t)), static_cast<Uint8>(180 * t), static_cast<Uint8>(255 * t), 255};
}

static double mandelbrotSmooth(double x0, double y0, int maxIterations) {
    double x = 0.0;
    double y = 0.0;
    int iter = 0;
    while (x * x + y * y <= 4.0 && iter < maxIterations) {
        const double xt = x * x - y * y + x0;
        y = 2.0 * x * y + y0;
        x = xt;
        ++iter;
    }
    if (iter == maxIterations) {
        return static_cast<double>(maxIterations);
    }
    const double mag2 = x * x + y * y;
    const double nu = iter + 1.0 - std::log2(std::log2(std::max(mag2, 1e-12)));
    return std::clamp(nu, 0.0, static_cast<double>(maxIterations));
}

static const char* paletteName(PaletteMode p) {
    switch (p) {
        case PaletteMode::Classic: return "classic";
        case PaletteMode::Fire: return "fire";
        case PaletteMode::Ice: return "ice";
    }
    return "unknown";
}

static bool saveScreenshot(SDL_Renderer* renderer, const std::string& filename) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return false;
    const int readRes = SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch);
    if (readRes != 0) {
        SDL_FreeSurface(surface);
        return false;
    }
    const int saveRes = SDL_SaveBMP(surface, filename.c_str());
    SDL_FreeSurface(surface);
    return saveRes == 0;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (!win) return 1;
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    ViewState view;
    PaletteMode palette = PaletteMode::Classic;
    int manualIterationOffset = 0;
    int screenshotIndex = 1;
    bool captureScreenshot = false;
    bool viewDirty = true;
    bool dragging = false;
    int dragLastX = 0;
    int dragLastY = 0;

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_r: view = ViewState{}; manualIterationOffset = 0; viewDirty = true; break;
                    case SDLK_s: captureScreenshot = true; break;
                    case SDLK_c: palette = static_cast<PaletteMode>((static_cast<int>(palette) + 1) % 3); viewDirty = true; break;
                    case SDLK_LEFT: view.centerX -= PAN_STEP / view.zoom; viewDirty = true; break;
                    case SDLK_RIGHT: view.centerX += PAN_STEP / view.zoom; viewDirty = true; break;
                    case SDLK_UP: view.centerY -= PAN_STEP / view.zoom; viewDirty = true; break;
                    case SDLK_DOWN: view.centerY += PAN_STEP / view.zoom; viewDirty = true; break;
                    case SDLK_LEFTBRACKET: manualIterationOffset -= 30; viewDirty = true; break;
                    case SDLK_RIGHTBRACKET: manualIterationOffset += 30; viewDirty = true; break;
                    case SDLK_EQUALS:
                    case SDLK_PLUS: applyCursorZoom(WIDTH / 2, HEIGHT / 2, ZOOM_FACTOR_IN, view); viewDirty = true; break;
                    case SDLK_MINUS: applyCursorZoom(WIDTH / 2, HEIGHT / 2, ZOOM_FACTOR_OUT, view); viewDirty = true; break;
                    default: break;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                int mx = 0, my = 0;
                SDL_GetMouseState(&mx, &my);
                if (event.wheel.y > 0) applyCursorZoom(mx, my, ZOOM_FACTOR_IN, view);
                if (event.wheel.y < 0) applyCursorZoom(mx, my, ZOOM_FACTOR_OUT, view);
                viewDirty = true;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                dragging = true;
                dragLastX = event.button.x;
                dragLastY = event.button.y;
            } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
            } else if (event.type == SDL_MOUSEMOTION && dragging) {
                const int dx = event.motion.x - dragLastX;
                const int dy = event.motion.y - dragLastY;
                view.centerX -= dx * viewportWidth(view) / WIDTH;
                view.centerY -= dy * viewportHeight(view) / HEIGHT;
                dragLastX = event.motion.x;
                dragLastY = event.motion.y;
                viewDirty = true;
            }
        }

        if (viewDirty) {
            const int maxIterations = adaptiveIterations(view.zoom, manualIterationOffset);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            for (int py = 0; py < HEIGHT; ++py) {
                for (int px = 0; px < WIDTH; ++px) {
                    const double x0 = screenToComplexX(px, view);
                    const double y0 = screenToComplexY(py, view);
                    const double smoothIter = mandelbrotSmooth(x0, y0, maxIterations);
                    if (smoothIter >= maxIterations) {
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    } else {
                        const double t = smoothIter / maxIterations;
                        const SDL_Color c = paletteColor(t, palette);
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                    }
                    SDL_RenderDrawPoint(renderer, px, py);
                }
            }

            if (captureScreenshot) {
                const std::string filename = "screenshot_" + std::to_string(screenshotIndex++) + ".bmp";
                if (!saveScreenshot(renderer, filename)) {
                    std::cout << "Failed screenshot: " << SDL_GetError() << std::endl;
                }
                captureScreenshot = false;
            }

            SDL_RenderPresent(renderer);

            std::ostringstream title;
            title << std::fixed << std::setprecision(6)
                  << "Mandelbrot | zoom=" << view.zoom
                  << " center=(" << view.centerX << "," << view.centerY << ")"
                  << " iter=" << maxIterations
                  << " palette=" << paletteName(palette)
                  << " | wheel zoom, drag/arrows pan, R reset, S shot, [ ] iter, C palette, Esc quit";
            SDL_SetWindowTitle(win, title.str().c_str());
            viewDirty = false;
        }
        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
