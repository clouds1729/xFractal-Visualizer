#include <SDL.h>
#include <iostream>
#include <cmath>

#define WIDTH 800
#define HEIGHT 600

const int MAX_ITER = 1000;

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
        return { 255, static_cast<Uint8>(t), 0, 255 };
    case 1:
        return { static_cast<Uint8>(q), 255, 0, 255 };
    case 2:
        return { 0, 255, static_cast<Uint8>(t), 255 };
    case 3:
        return { 0, static_cast<Uint8>(q), 255, 255 };
    case 4:
        return { static_cast<Uint8>(t), 0, 255, 255 };
    default:
        return { 255, 0, static_cast<Uint8>(q), 255 };
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

int main(int argc, char* argv[]) {
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create the window and renderer.
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

    // Main loop.
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                // Zoom in/out
                if (x >= 10 && x <= 60 && y >= 10 && y <= 60) {
                    zoom *= 1.1; // Zoom in by 10%
                }
                else if (x >= 70 && x <= 120 && y >= 10 && y <= 60) {
                    zoom /= 1.1; // Zoom out by 10%
                }

                // Move buttons
                else if (x >= 10 && x <= 60 && y >= 70 && y <= 120) {
                    offsetX -= 0.1 / zoom; // Move left
                }
                else if (x >= 70 && x <= 120 && y >= 70 && y <= 120) {
                    offsetX += 0.1 / zoom; // Move right
                }
                else if (x >= 130 && x <= 180 && y >= 70 && y <= 120) {
                    offsetY -= 0.1 / zoom; // Move up
                }
                else if (x >= 190 && x <= 240 && y >= 70 && y <= 120) {
                    offsetY += 0.1 / zoom; // Move down
                }
            }
        }

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Calculate and render the Mandelbrot set with zoom and offset
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                double x0 = map(x, 0, WIDTH, -2.5 / zoom + offsetX, 1.0 / zoom + offsetX);
                double y0 = map(y, 0, HEIGHT, -1.0 / zoom + offsetY, 1.0 / zoom + offsetY);

                int color = mandelbrot(x0, y0);
                double hue = fmod(color * 10.0, 360.0); // Map color to rainbow
                SDL_SetRenderDrawColor(renderer, hueToRGB(hue).r, hueToRGB(hue).g, hueToRGB(hue).b, 255);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }

        // Render zoom in/out buttons
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect zoomInRect = { 10, 10, 50, 50 };
        SDL_RenderFillRect(renderer, &zoomInRect);
        SDL_Rect zoomOutRect = { 70, 10, 50, 50 };
        SDL_RenderFillRect(renderer, &zoomOutRect);

        // Update the screen
        SDL_RenderPresent(renderer);

        // Print current zoom level
        std::cout << "Zoom: " << zoom << std::endl;
    }

    // Clean up.
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
