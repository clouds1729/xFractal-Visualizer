# xFractal-Visualizer

Interactive Mandelbrot explorer built with C++ and SDL2.
## Image
<img width="2424" height="1896" alt="image" src="https://github.com/user-attachments/assets/addd1bcb-098b-49fb-a954-3b42f1504bf0" />

<img width="2410" height="1876" alt="image" src="https://github.com/user-attachments/assets/6d42f67f-15a7-4ee4-a6c0-2016fa9e004c" />


## Controls

### Keyboard controls

| Input | Action |
|---|---|
| Arrow keys | Pan view |
| `R` | Reset center/zoom/iteration offset |
| `+` / `=` | Zoom in (toward center) |
| `-` | Zoom out (toward center) |
| `[` / `]` | Decrease / increase iteration detail |
| `C` | Cycle named palettes (fire/lava, ocean/ice, neon, grayscale, rainbow) |
| `V` | Reverse current palette |
| `,` / `.` | Decrease / increase color contrast/intensity |
| `S` | Save BMP screenshot |
| `Esc` | Quit |

### Mouse controls

| Input | Action |
|---|---|
| Mouse wheel | Zoom toward cursor |
| Left click + drag | Pan view |

### On-screen button controls (SDL overlay)

A lightweight right-side overlay provides clickable buttons for:

- `Zoom +`
- `Zoom -`
- `Pan ←`
- `Pan →`
- `Pan ↑`
- `Pan ↓`
- `Reset`
- `Detail +`
- `Detail -`
- `Palette`

The button overlay reuses the same navigation/detail/palette logic as keyboard controls.

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

## Limitations of the simple overlay

- The overlay uses SDL primitive shapes (rectangles/line icons), not font-rendered text, to stay lightweight.
- Because there is no `SDL_ttf` dependency, detailed button labels are documented here and summarized in the window title/status.
- The panel intentionally occupies a small right-side strip and may still cover a small part of the fractal view.
