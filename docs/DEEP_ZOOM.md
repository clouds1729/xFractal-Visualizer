# Deep Zoom Architecture Plan

## 1) Why double precision fails

`double` gives ~53 bits of mantissa (~15-16 decimal digits). During deep zoom, pixel-to-pixel coordinate deltas become smaller than representable spacing around the current center. Neighboring pixels then map to identical complex coordinates, so detail collapses and the image looks flat or noisy.

## 2) Why arbitrary precision helps (and costs)

Arbitrary precision types (e.g., `boost::multiprecision`) allow much smaller coordinate steps, extending zoom depth drastically. Cost: arithmetic is much slower than hardware doubles, so naive per-pixel high-precision iteration can reduce interactivity.

## 3) Perturbation rendering (high level)

Perturbation computes one high-precision reference orbit for a tile/view center, then computes nearby pixels with fast double-precision delta updates. This keeps precision where needed while preserving speed for most pixels.

## 4) Series approximation acceleration

Series approximation expands orbit behavior near the reference point (Taylor-like expansion). Many pixels can be evaluated from the expansion rather than full iteration, reducing total compute and improving deep zoom responsiveness.

## 5) Staged implementation plan

1. **Stage 1: ViewState abstraction**
   - Keep center/zoom mapping in one class.
   - Completed direction in current codebase (`ViewState`, mapping helpers).
2. **Stage 2: Adaptive iterations + smooth coloring**
   - Increase iterations with zoom and use fractional escape-time coloring.
   - Completed direction in current codebase.
3. **Stage 3: boost::multiprecision coordinates**
   - Introduce precision alias (`using Real = ...`) and migrate view-coordinate math first.
   - Keep pixel loop in double as fallback mode.
4. **Stage 4: Perturbation rendering**
   - Add reference orbit generator in high precision.
   - Add delta evaluator for per-pixel doubles.
5. **Stage 5: tiled cache + progressive refinement**
   - Render coarse tiles first, then refine.
   - Cache tiles keyed by view hash and iteration/palette settings.

## 6) Proposed files/classes

- `include/ViewState.h` / `src/ViewState.cpp` for coordinate mapping and zoom/pan state.
- `include/FractalParams.h` for iteration/palette/detail settings.
- `include/MandelbrotRenderer.h` / `src/MandelbrotRenderer.cpp` for current double renderer.
- `include/DeepZoomRenderer.h` / `src/DeepZoomRenderer.cpp` for perturbation pipeline.
- `include/Precision.h` for compile-time precision alias and conversion utilities.

## 7) What can be tested without a window

- Coordinate mapping invariants (screen center maps to view center).
- Cursor-anchored zoom invariant (complex coordinate under cursor remains stable).
- Adaptive iteration monotonicity (more zoom => >= iterations).
- Palette/color mapping value range [0,255].
- Deep-zoom warning threshold logic (if/when kept in title/status formatter).
- View hash stability for tile-cache keys.

## Practical tradeoff summary

- **Double only**: fastest, simplest, limited deep zoom.
- **Full multiprecision per pixel**: most direct precision boost, likely too slow for interactive 800x600.
- **Perturbation + series**: best path for real deep zoom at interactive speed, but highest implementation complexity.
