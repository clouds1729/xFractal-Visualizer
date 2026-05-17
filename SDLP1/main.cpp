#include <SDL.h>
#include <cmath>
#include <iostream>
#include <string>

#define WIDTH 800
#define HEIGHT 600

const int MAX_ITER = 1000;

constexpr double X_MIN = -2.5;
constexpr double X_MAX = 1.0;
constexpr double Y_MIN = -1.0;
constexpr double Y_MAX = 1.0;

constexpr double ZOOM_FACTOR_IN = 1.1;
constexpr double ZOOM_FACTOR_OUT = 1.0 / ZOOM_FACTOR_IN;
constexpr double PAN_STEP = 0.1;

double map(double value, double in_min, double in_max, double out_min, double out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

SDL_Color hueToRGB(double hue) {
    double h = hue / 60.0;
    int i = static_cast<int>(h);
    double f = h - i;
    double p = 255 * (1.0 - 1.0);
    double q = 255 * (1.0 - f);
    double t = 255 * (1.0 - (1.0 - f));

    switch (i) {
    case 0:
        return {255, static_cast<Uint8>(t), 0, 255};
    case 1:
        return {static_cast<Uint8>(q), 255, 0, 255};
    case 2:
        return {0, 255, static_cast<Uint8>(t), 255};
    case 3:
        return {0, static_cast<Uint8>(q), 255, 255};
    case 4:
        return {static_cast<Uint8>(t), 0, 255, 255};
    default:
        return {255, 0, static_cast<Uint8>(q), 255};
    }
}

int mandelbrot(double x0, double y0) {
    double x = 0.0, y = 0.0;
    int iter = 0;
    while (x * x + y * y < 4 && iter < MAX_ITER) {
        double xtemp = x * x - y * y + x0;
        y = 2 * x * y + y0;
        x = xtemp;
        iter++;
    }
    return iter;
}

double screenToComplexX(int x, double zoom, double offsetX) {
    return map(x, 0, WIDTH, X_MIN / zoom + offsetX, X_MAX / zoom + offsetX);
}

double screenToComplexY(int y, double zoom, double offsetY) {
    return map(y, 0, HEIGHT, Y_MIN / zoom + offsetY, Y_MAX / zoom + offsetY);
}

void applyCursorZoom(int mouseX, int mouseY, double zoomFactor, double& zoom, double& offsetX, double& offsetY) {
    const double beforeX = screenToComplexX(mouseX, zoom, offsetX);
    const double beforeY = screenToComplexY(mouseY, zoom, offsetY);

    zoom *= zoomFactor;

    const double mappedX = screenToComplexX(mouseX, zoom, offsetX);
    const double mappedY = screenToComplexY(mouseY, zoom, offsetY);

    offsetX += beforeX - mappedX;
    offsetY += beforeY - mappedY;
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

    double zoom = 1.0;
    double offsetX = 0.0;
    double offsetY = 0.0;
    int screenshotIndex = 1;

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
                    zoom = 1.0;
                    offsetX = 0.0;
                    offsetY = 0.0;
                    break;
                case SDLK_s: {
                    const std::string filename = "screenshot_" + std::to_string(screenshotIndex++) + ".bmp";
                    if (!saveScreenshot(renderer, filename)) {
                        std::cout << "Failed to save screenshot: " << SDL_GetError() << std::endl;
                    }
                    break;
                }
                case SDLK_LEFT:
                    offsetX -= PAN_STEP / zoom;
                    break;
                case SDLK_RIGHT:
                    offsetX += PAN_STEP / zoom;
                    break;
                case SDLK_UP:
                    offsetY -= PAN_STEP / zoom;
                    break;
                case SDLK_DOWN:
                    offsetY += PAN_STEP / zoom;
                    break;
                default:
                    break;
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                int mouseX = 0;
                int mouseY = 0;
                SDL_GetMouseState(&mouseX, &mouseY);
                if (event.wheel.y > 0) {
                    applyCursorZoom(mouseX, mouseY, ZOOM_FACTOR_IN, zoom, offsetX, offsetY);
                } else if (event.wheel.y < 0) {
                    applyCursorZoom(mouseX, mouseY, ZOOM_FACTOR_OUT, zoom, offsetX, offsetY);
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                if (event.button.button == SDL_BUTTON_LEFT) {
                    applyCursorZoom(x, y, ZOOM_FACTOR_IN, zoom, offsetX, offsetY);
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    applyCursorZoom(x, y, ZOOM_FACTOR_OUT, zoom, offsetX, offsetY);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                double x0 = screenToComplexX(x, zoom, offsetX);
                double y0 = screenToComplexY(y, zoom, offsetY);

                int color = mandelbrot(x0, y0);
                double hue = fmod(color * 10.0, 360.0);
                SDL_Color rgb = hueToRGB(hue);
                SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
