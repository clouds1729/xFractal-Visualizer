#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define WIDTH 800
#define HEIGHT 600

using Precision = double;

constexpr Precision X_MIN = static_cast<Precision>(-2.5);
constexpr Precision X_MAX = static_cast<Precision>(1.0);
constexpr Precision Y_MIN = static_cast<Precision>(-1.0);
constexpr Precision Y_MAX = static_cast<Precision>(1.0);

constexpr Precision ZOOM_FACTOR_IN = static_cast<Precision>(1.12);
constexpr Precision ZOOM_FACTOR_OUT = static_cast<Precision>(1.0) / ZOOM_FACTOR_IN;
constexpr Precision PAN_STEP = static_cast<Precision>(0.08);
constexpr Precision DOUBLE_WARNING_ZOOM = static_cast<Precision>(1e12);

struct ViewState {
    Precision zoom = static_cast<Precision>(1.0);
    Precision centerX = (X_MIN + X_MAX) * static_cast<Precision>(0.5);
    Precision centerY = (Y_MIN + Y_MAX) * static_cast<Precision>(0.5);
};

enum class PaletteMode { Fire = 0, OceanIce = 1, Neon = 2, Grayscale = 3, Rainbow = 4 };

enum class ButtonAction { ZoomIn, ZoomOut, PanLeft, PanRight, PanUp, PanDown, Reset, DetailUp, DetailDown, PaletteNext };

struct OverlayButton {
    SDL_Rect rect{};
    ButtonAction action;
    const char* label;
};

static Precision mapValue(Precision value, Precision inMin, Precision inMax, Precision outMin, Precision outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

static Precision viewportWidth(const ViewState& view) { return (X_MAX - X_MIN) / view.zoom; }
static Precision viewportHeight(const ViewState& view) { return (Y_MAX - Y_MIN) / view.zoom; }

static Precision screenToComplexX(int x, const ViewState& view) {
    const Precision halfW = viewportWidth(view) * static_cast<Precision>(0.5);
    return mapValue(static_cast<Precision>(x), static_cast<Precision>(0.0), static_cast<Precision>(WIDTH), view.centerX - halfW, view.centerX + halfW);
}

static Precision screenToComplexY(int y, const ViewState& view) {
    const Precision halfH = viewportHeight(view) * static_cast<Precision>(0.5);
    return mapValue(static_cast<Precision>(y), static_cast<Precision>(0.0), static_cast<Precision>(HEIGHT), view.centerY - halfH, view.centerY + halfH);
}

static void applyCursorZoom(int mouseX, int mouseY, Precision factor, ViewState& view) {
    const Precision beforeX = screenToComplexX(mouseX, view);
    const Precision beforeY = screenToComplexY(mouseY, view);

    view.zoom *= factor;

    const Precision afterX = screenToComplexX(mouseX, view);
    const Precision afterY = screenToComplexY(mouseY, view);

    view.centerX += beforeX - afterX;
    view.centerY += beforeY - afterY;
}

static int adaptiveIterations(Precision zoom, int manualOffset) {
    const Precision logZoom = std::max(static_cast<Precision>(0.0), std::log10(zoom));
    const int base = 120 + static_cast<int>(logZoom * static_cast<Precision>(90.0));
    return std::clamp(base + manualOffset, 50, 4000);
}

static Uint8 adjustContrast(double value, double intensity) {
    const double centered = (value - 127.5) * intensity + 127.5;
    return static_cast<Uint8>(std::clamp(centered, 0.0, 255.0));
}

static SDL_Color paletteColor(double t, PaletteMode palette, double intensity, bool reversePalette) {
    t = std::clamp(t, 0.0, 1.0);
    if (reversePalette) t = 1.0 - t;

    double r = 0.0, g = 0.0, b = 0.0;
    switch (palette) {
        case PaletteMode::Fire:
            r = 255.0 * std::pow(t, 0.65);
            g = 220.0 * std::pow(t, 1.2);
            b = 90.0 * std::pow(1.0 - t, 2.2);
            break;
        case PaletteMode::OceanIce:
            r = 40.0 + 70.0 * t;
            g = 80.0 + 150.0 * std::pow(t, 0.9);
            b = 120.0 + 135.0 * std::pow(t, 0.45);
            break;
        case PaletteMode::Neon:
            r = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.00));
            g = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.33));
            b = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.66));
            break;
        case PaletteMode::Grayscale:
            r = g = b = 255.0 * t;
            break;
        case PaletteMode::Rainbow:
            r = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.00));
            g = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.25));
            b = 127.0 + 127.0 * std::sin(6.28318 * (t + 0.50));
            break;
    }

    return {
        adjustContrast(r, intensity),
        adjustContrast(g, intensity),
        adjustContrast(b, intensity),
        255
    };
}

static Precision mandelbrotSmooth(Precision x0, Precision y0, int maxIterations) {
    Precision x = static_cast<Precision>(0.0);
    Precision y = static_cast<Precision>(0.0);
    int iter = 0;
    while (x * x + y * y <= static_cast<Precision>(4.0) && iter < maxIterations) {
        const Precision xt = x * x - y * y + x0;
        y = static_cast<Precision>(2.0) * x * y + y0;
        x = xt;
        ++iter;
    }
    if (iter == maxIterations) {
        return static_cast<Precision>(maxIterations);
    }
    const Precision mag2 = x * x + y * y;
    const Precision nu = static_cast<Precision>(iter) + static_cast<Precision>(1.0) - std::log2(std::log2(std::max(mag2, static_cast<Precision>(1e-12))));
    return std::clamp(nu, static_cast<Precision>(0.0), static_cast<Precision>(maxIterations));
}

static const char* paletteName(PaletteMode p) {
    switch (p) {
        case PaletteMode::Fire: return "fire/lava";
        case PaletteMode::OceanIce: return "ocean/ice";
        case PaletteMode::Neon: return "neon";
        case PaletteMode::Grayscale: return "grayscale";
        case PaletteMode::Rainbow: return "rainbow";
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

static std::vector<OverlayButton> buildOverlayButtons() {
    constexpr int panelX = WIDTH - 138;
    constexpr int panelY = 12;
    constexpr int buttonW = 124;
    constexpr int buttonH = 28;
    constexpr int gap = 6;

    std::vector<OverlayButton> buttons;
    const ButtonAction actions[] = {ButtonAction::ZoomIn, ButtonAction::ZoomOut, ButtonAction::PanLeft, ButtonAction::PanRight, ButtonAction::PanUp, ButtonAction::PanDown,
                                    ButtonAction::Reset, ButtonAction::DetailUp, ButtonAction::DetailDown, ButtonAction::PaletteNext};
    const char* labels[] = {"Zoom +", "Zoom -", "Left", "Right", "Up", "Down", "Reset", "Detail +", "Detail -", "Palette"};

    for (int i = 0; i < 10; ++i) {
        buttons.push_back({SDL_Rect{panelX, panelY + i * (buttonH + gap), buttonW, buttonH}, actions[i], labels[i]});
    }
    return buttons;
}

static void drawButtonIcon(SDL_Renderer* renderer, const SDL_Rect& rect, ButtonAction action) {
    const int cx = rect.x + rect.w / 2;
    const int cy = rect.y + rect.h / 2;
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    switch (action) {
        case ButtonAction::ZoomIn:
            SDL_RenderDrawLine(renderer, cx - 7, cy, cx + 7, cy);
            SDL_RenderDrawLine(renderer, cx, cy - 7, cx, cy + 7);
            break;
        case ButtonAction::ZoomOut:
            SDL_RenderDrawLine(renderer, cx - 7, cy, cx + 7, cy);
            break;
        case ButtonAction::PanLeft:
            SDL_RenderDrawLine(renderer, cx + 7, cy - 7, cx - 7, cy);
            SDL_RenderDrawLine(renderer, cx + 7, cy + 7, cx - 7, cy);
            break;
        case ButtonAction::PanRight:
            SDL_RenderDrawLine(renderer, cx - 7, cy - 7, cx + 7, cy);
            SDL_RenderDrawLine(renderer, cx - 7, cy + 7, cx + 7, cy);
            break;
        case ButtonAction::PanUp:
            SDL_RenderDrawLine(renderer, cx - 7, cy + 7, cx, cy - 7);
            SDL_RenderDrawLine(renderer, cx + 7, cy + 7, cx, cy - 7);
            break;
        case ButtonAction::PanDown:
            SDL_RenderDrawLine(renderer, cx - 7, cy - 7, cx, cy + 7);
            SDL_RenderDrawLine(renderer, cx + 7, cy - 7, cx, cy + 7);
            break;
        case ButtonAction::Reset:
            {
                SDL_Rect square{cx - 7, cy - 7, 14, 14};
                SDL_RenderDrawRect(renderer, &square);
            }
            break;
        case ButtonAction::DetailUp:
            SDL_RenderDrawLine(renderer, cx - 8, cy + 5, cx, cy - 5);
            SDL_RenderDrawLine(renderer, cx, cy - 5, cx + 8, cy + 5);
            SDL_RenderDrawLine(renderer, cx - 5, cy + 5, cx + 5, cy + 5);
            break;
        case ButtonAction::DetailDown:
            SDL_RenderDrawLine(renderer, cx - 8, cy - 5, cx, cy + 5);
            SDL_RenderDrawLine(renderer, cx, cy + 5, cx + 8, cy - 5);
            SDL_RenderDrawLine(renderer, cx - 5, cy - 5, cx + 5, cy - 5);
            break;
        case ButtonAction::PaletteNext:
            SDL_RenderDrawLine(renderer, cx - 8, cy, cx + 8, cy);
            SDL_RenderDrawLine(renderer, cx + 8, cy, cx + 4, cy - 4);
            SDL_RenderDrawLine(renderer, cx + 8, cy, cx + 4, cy + 4);
            break;
    }
}

static bool applyAction(ButtonAction action, ViewState& view, int& manualIterationOffset, PaletteMode& palette, bool& reversePalette, bool& captureScreenshot, double& colorIntensity) {
    switch (action) {
        case ButtonAction::ZoomIn: applyCursorZoom(WIDTH / 2, HEIGHT / 2, ZOOM_FACTOR_IN, view); return true;
        case ButtonAction::ZoomOut: applyCursorZoom(WIDTH / 2, HEIGHT / 2, ZOOM_FACTOR_OUT, view); return true;
        case ButtonAction::PanLeft: view.centerX -= PAN_STEP / view.zoom; return true;
        case ButtonAction::PanRight: view.centerX += PAN_STEP / view.zoom; return true;
        case ButtonAction::PanUp: view.centerY -= PAN_STEP / view.zoom; return true;
        case ButtonAction::PanDown: view.centerY += PAN_STEP / view.zoom; return true;
        case ButtonAction::Reset: view = ViewState{}; manualIterationOffset = 0; return true;
        case ButtonAction::DetailUp: manualIterationOffset += 30; return true;
        case ButtonAction::DetailDown: manualIterationOffset -= 30; return true;
        case ButtonAction::PaletteNext:
            palette = static_cast<PaletteMode>((static_cast<int>(palette) + 1) % 5);
            return true;
    }
    (void)reversePalette;
    (void)captureScreenshot;
    (void)colorIntensity;
    return false;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (!win) return 1;
    SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    ViewState view;
    PaletteMode palette = PaletteMode::Fire;
    int manualIterationOffset = 0;
    int screenshotIndex = 1;
    bool captureScreenshot = false;
    bool reversePalette = false;
    double colorIntensity = 1.0;
    bool viewDirty = true;
    bool dragging = false;
    int dragLastX = 0;
    int dragLastY = 0;
    const SDL_Color insideColor{0, 0, 0, 255};
    const std::vector<OverlayButton> buttons = buildOverlayButtons();

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
                    case SDLK_c: palette = static_cast<PaletteMode>((static_cast<int>(palette) + 1) % 5); viewDirty = true; break;
                    case SDLK_v: reversePalette = !reversePalette; viewDirty = true; break;
                    case SDLK_COMMA: colorIntensity = std::max(0.40, colorIntensity - 0.10); viewDirty = true; break;
                    case SDLK_PERIOD: colorIntensity = std::min(3.00, colorIntensity + 0.10); viewDirty = true; break;
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
                bool clickedButton = false;
                for (const OverlayButton& button : buttons) {
                    if (event.button.x >= button.rect.x && event.button.x < button.rect.x + button.rect.w && event.button.y >= button.rect.y && event.button.y < button.rect.y + button.rect.h) {
                        viewDirty = applyAction(button.action, view, manualIterationOffset, palette, reversePalette, captureScreenshot, colorIntensity) || viewDirty;
                        clickedButton = true;
                        break;
                    }
                }
                if (!clickedButton) {
                    dragging = true;
                    dragLastX = event.button.x;
                    dragLastY = event.button.y;
                }
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
                    const Precision x0 = screenToComplexX(px, view);
                    const Precision y0 = screenToComplexY(py, view);
                    const Precision smoothIter = mandelbrotSmooth(x0, y0, maxIterations);
                    if (smoothIter >= maxIterations) {
                        SDL_SetRenderDrawColor(renderer, insideColor.r, insideColor.g, insideColor.b, 255);
                    } else {
                        const double t = static_cast<double>(smoothIter / static_cast<Precision>(maxIterations));
                        const SDL_Color c = paletteColor(t, palette, colorIntensity, reversePalette);
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                    }
                    SDL_RenderDrawPoint(renderer, px, py);
                }
            }

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 180);
            const SDL_Rect panelRect{WIDTH - 144, 8, 136, 352};
            SDL_RenderFillRect(renderer, &panelRect);
            for (const OverlayButton& button : buttons) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
                SDL_RenderDrawRect(renderer, &button.rect);
                drawButtonIcon(renderer, button.rect, button.action);
            }

            if (captureScreenshot) {
                const std::string filename = "screenshot_" + std::to_string(screenshotIndex++) + ".bmp";
                if (!saveScreenshot(renderer, filename)) {
                    std::cout << "Failed screenshot: " << SDL_GetError() << std::endl;
                }
                captureScreenshot = false;
            }

            SDL_RenderPresent(renderer);

            const bool doublePrecisionLimitNear = view.zoom >= DOUBLE_WARNING_ZOOM;

            std::ostringstream title;
            title << std::fixed << std::setprecision(6)
                  << "Mandelbrot | palette=" << paletteName(palette)
                  << (reversePalette ? " (rev)" : "")
                  << " contrast=" << std::setprecision(2) << colorIntensity
                  << std::setprecision(6)
                  << " zoom=" << view.zoom
                  << " center=(" << view.centerX << "," << view.centerY << ")"
                  << " iter=" << maxIterations
                  << " | panel: + - < > ^ v [] C";
            if (doublePrecisionLimitNear) {
                title << " WARNING: double precision limit near";
            }
            title << " | wheel zoom, drag/arrows pan, R reset, S shot, [ ] iter, C palette, V reverse, ,/. contrast";
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
