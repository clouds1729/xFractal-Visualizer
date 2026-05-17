# xFractal-Visualizer

An interactive Mandelbrot fractal visualizer written in C++ with SDL2.

## Highlights

- Real-time Mandelbrot rendering.
- Mouse-centered zoom (wheel, left-click zoom in, right-click zoom out).
- Keyboard and arrow-key panning controls.
- Quick view reset and BMP screenshot export.

## Controls

- **Mouse wheel up / Left click**: Zoom in toward cursor.
- **Mouse wheel down / Right click**: Zoom out from cursor.
- **Arrow keys**: Pan view.
- **R**: Reset to default view.
- **S**: Save screenshot as `screenshot_<n>.bmp` in the working directory.
- **Esc**: Quit.

## Build (CMake)

This project now supports a standard CMake workflow.

### Linux (Ubuntu/Debian example)

```bash
sudo apt update
sudo apt install -y cmake g++ libsdl2-dev
cmake -S . -B build
cmake --build build
./build/xfractal_visualizer
```

### macOS (Homebrew)

```bash
brew install cmake sdl2
cmake -S . -B build
cmake --build build
./build/xfractal_visualizer
```

### Windows

Option A: **Visual Studio + CMake**

1. Install Visual Studio (Desktop development with C++).
2. Install SDL2 development files (or via a package manager such as vcpkg).
3. Configure and build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\xfractal_visualizer.exe
```

Option B: **vcpkg** example

```powershell
vcpkg install sdl2:x64-windows
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Project Structure

```text
.
├── CMakeLists.txt          # Cross-platform CMake build file
├── README.md               # Project documentation
├── LICENSE                 # MIT license
├── SDLP1.sln               # Existing Visual Studio solution
└── SDLP1/
    ├── main.cpp            # SDL2 app + Mandelbrot rendering and interaction logic
    ├── SDLP1.vcxproj       # Existing Visual Studio project
    └── *.dll               # Runtime DLLs currently tracked in repo
```

## Roadmap

- [ ] Improve color palettes and smooth coloring.
- [ ] Add frame-time/FPS overlay and optional HUD.
- [ ] Add resolution/window-size options.
- [ ] Add Julia set mode.
- [ ] Add image export formats beyond BMP (PNG).
- [ ] Add CI for multi-platform build verification.

## Notes

- The fractal computation is currently single-threaded.
- Screenshot output uses SDL BMP writing for portability.

## License

MIT — see [LICENSE](LICENSE).
