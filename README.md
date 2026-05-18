# xFractal-Visualizer

Interactive Mandelbrot explorer built with C++ and SDL2.

## Controls

| Input | Action |
|---|---|
| Mouse wheel | Zoom toward cursor |
| Left click + drag | Pan view |
| Arrow keys | Pan view |
| `R` | Reset center/zoom/iteration offset |
| `+` / `=` | Zoom in |
| `-` | Zoom out |
| `[` / `]` | Decrease / increase iteration detail |
| `C` | Cycle palette |
| `S` | Save BMP screenshot |
| `Esc` | Quit |

## Build

### Ubuntu

```bash
sudo apt update
sudo apt install -y cmake g++ libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/xfractal_visualizer
```

### Windows (Visual Studio)

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\xfractal_visualizer.exe
```

## Adaptive detail

The explorer chooses `maxIterations` automatically from zoom level, then applies a manual offset from `[` and `]`. This keeps boundary detail visible when zooming without forcing high cost at shallow zoom.

## Deep zoom limitations

This build uses `double` precision coordinates. At extreme zoom depths, floating-point precision eventually collapses nearby coordinates into the same value, causing visual artifacts/flat regions. True "infinite" zoom generally needs arbitrary precision and/or perturbation rendering. See `docs/DEEP_ZOOM.md` for the staged design.
