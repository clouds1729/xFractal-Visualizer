# Mandelbrot Set Visualizer

The Mandelbrot Set Visualizer is a C++ program that uses the SDL (Simple DirectMedia Layer) library to render the Mandelbrot fractal set. The program allows users to zoom in and out, as well as pan around the fractal, providing an interactive way to explore this fascinating mathematical concept.

## Features

1. **Mandelbrot Set Visualization**: The program calculates and renders the Mandelbrot set, using a color palette to represent the number of iterations required for each point.
2. **Zoom In/Out**: Users can zoom in and out of the fractal by clicking on the corresponding buttons in the top-left corner of the window.
3. **Panning**: Users can pan around the fractal by clicking on the corresponding buttons in the bottom-left corner of the window.
4. **Real-time Updates**: The program updates the fractal visualization in real-time as the user interacts with the window.

## Requirements

To run this program, you will need the following:

- C++ compiler (e.g., GCC, Clang)
- SDL2 library installed on your system

## How to Build and Run

1. Clone the repository:
   ```
   git clone https://github.com/your-username/mandelbrot-visualizer.git
   ```

2. Navigate to the project directory:
   ```
   cd mandelbrot-visualizer
   ```

3. Compile the program:
   ```
   g++ -o mandelbrot main.cpp -lSDL2
   ```

4. Run the program:
   ```
   ./mandelbrot
   ```

   The Mandelbrot Set Visualizer window should now be displayed.

## Usage

1. **Zoom In/Out**: Click the "+" and "-" buttons in the top-left corner of the window to zoom in and out of the fractal.
2. **Panning**: Click the arrow buttons in the bottom-left corner of the window to pan around the fractal.
3. **Quit**: Press the "X" button in the top-right corner of the window to quit the program.

## Code Overview

The main components of the code are:

1. **SDL Initialization**: The program initializes the SDL library, creates a window, and a renderer.
2. **Mandelbrot Set Calculation**: The `mandelbrot` function calculates the number of iterations required for a given point in the complex plane to escape the Mandelbrot set.
3. **Color Conversion**: The `hueToRGB` function converts a hue value to an RGB color.
4. **Mapping Functions**: The `map` function maps a value from one range to another.
5. **Main Loop**: The program's main loop handles user events (e.g., zooming, panning) and updates the fractal visualization accordingly.

## Contribution

If you find any issues or have suggestions for improvements, please feel free to open an issue or submit a pull request. Contributions are always welcome!

## License

This project is licensed under the [MIT License](LICENSE).
