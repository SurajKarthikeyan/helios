# HELIOS Mission Control

A real-time NASA-style mission control terminal built in C using SDL2 and OpenGL, featuring CRT phosphor display effects, live ISS tracking, and system telemetry.

![HELIOS Screenshot](assets/screenshot.png)

## Features

- **CRT Shader Pipeline** — Full OpenGL post-processing with bloom, scanlines, screen curvature, phosphor flicker, and vignette
- **Live ISS Tracking** — Real-time International Space Station position from the Open Notify API, plotted on a 3D rotating Earth
- **Interactive Globe** — Auto-centers on the ISS when data arrives, then free-rotates with mouse drag
- **System Telemetry** — Live CPU usage, RAM usage, and system stats via Windows API
- **Alert Log** — Real-time severity-tiered event feed with timestamps
- **MET Clock** — Mission Elapsed Time counter
- **4 CRT Themes** — Phosphor Green, Amber, Cold Cyan, Minimal White
- **IBM VGA Font** — Authentic DOS-era bitmap font from int10h.org

## Screenshots

| Amber Theme | Phosphor Green |
|---|---|
| Globe tracking ISS over South America | Alert log with live system events |

## Building

### Prerequisites

- Windows 10/11 with MSYS2 (UCRT64)
- The following packages via pacman:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-SDL2 \
          mingw-w64-ucrt-x86_64-SDL2_ttf \
          mingw-w64-ucrt-x86_64-glew \
          mingw-w64-ucrt-x86_64-curl \
          mingw-w64-ucrt-x86_64-cjson \
          mingw-w64-ucrt-x86_64-stb \
          mingw-w64-ucrt-x86_64-make
```

### Build
```bash
git clone https://github.com/SurajKarthikeyan/helios
cd helios
mingw32-make
./helios.exe
```

### Assets Required

Download the IBM VGA 8x16 font from [int10h.org](https://int10h.org/oldschool-pc-fonts/) and place it at:
```
fonts/Px437_IBM_VGA_8x16.ttf
```

Download a 2K Earth texture (NASA Blue Marble or similar) and place it at:
```
assets/earth_map.jpg
```

These are not included in the repo due to licensing.

## Architecture
```
helios/
├── src/
│   ├── main.c       — SDL2/OpenGL init, render loop, UI canvas
│   ├── globe.c/h    — 3D Earth sphere, ISS marker, FBO rendering
│   ├── iss.c/h      — Live ISS position via Open Notify API (threaded)
│   ├── sysinfo.c/h  — Windows CPU/RAM telemetry
│   └── alert.c/h    — Circular alert log buffer
├── shaders/
│   ├── vert.glsl         — CRT quad vertex shader
│   ├── frag.glsl         — CRT effects (bloom, scanlines, curvature, flicker)
│   ├── globe_vert.glsl   — Sphere vertex shader
│   ├── globe_frag.glsl   — Land/water detection, theme tinting, lighting
│   ├── iss_vert.glsl     — ISS point marker vertex shader
│   └── iss_frag.glsl     — ISS glow effect fragment shader
├── fonts/                — IBM VGA font (not included)
├── assets/               — Earth texture (not included)
└── Makefile
```

## Tech Stack

- **C** — Core language
- **SDL2** — Window management, input, software canvas rendering
- **OpenGL 3.3 Core** — GPU shader pipeline, globe rendering
- **GLEW** — OpenGL extension loading
- **SDL2_ttf** — Font rendering to SDL surface
- **libcurl** — HTTP requests to ISS API
- **cJSON** — JSON parsing
- **stb_image** — Earth texture loading

## Data Sources

- ISS Position: [Open Notify API](http://api.open-notify.org/iss-now.json) — updates every 5 seconds
- System Stats: Windows `GetSystemTimes` and `GlobalMemoryStatusEx`

## License

MIT
