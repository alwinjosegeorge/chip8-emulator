# CHIP//8 Arcade — Developer & Contributor Guide

This guide details the development environment setup, build instructions, test harness execution, and procedures for extending the emulator and launcher.

---

## 1. Prerequisites

### Windows
* **C++ Compiler**: GCC (MinGW-w64 / w64devkit) supporting C++17.
* **Libraries**: `SDL2` development libraries (included or installed via MSYS2 / vcpkg).
* **Build Tool**: GNU Make or direct `g++` compilation.

### Linux (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install -y g++ make libsdl2-dev
```

### Linux (Arch)
```bash
sudo pacman -S gcc make sdl2
```

### macOS
```bash
brew install sdl2
```

---

## 2. Building from Source

### Quick Build with Make
```bash
# Build the main emulator application:
make

# Build and run the headless unit test harness:
make test

# Clean all build artifacts:
make clean
```

### Native Windows Build (without Make)
If building directly via PowerShell using MinGW-w64 / w64devkit:
```powershell
# Set compiler PATH (adjust path to your toolchain):
$env:PATH = "C:\Users\joshw\tools\w64devkit\bin;" + $env:PATH

# Compile the application:
g++ -std=c++17 -Wall -Wextra -O2 -I"C:\Users\joshw\tools\SDL2-2.30.10\x86_64-w64-mingw32\include" -L"C:\Users\joshw\tools\SDL2-2.30.10\x86_64-w64-mingw32\lib" -o chip8.exe src/main.cpp src/chip8.cpp src/ui_font.cpp src/app_state.cpp src/ui_renderer.cpp -lmingw32 -lSDL2main -lSDL2 -mconsole

# Compile and run unit tests:
g++ -std=c++17 -Wall -Wextra -O2 -o run_tests.exe tests/test_core.cpp src/chip8.cpp
.\run_tests.exe
```

---

## 3. Project Directory Organization

```
CHIP8-Arcade/
│
├── src/                          # C++ Source Code
│   ├── chip8.h                   # Virtual machine CPU & memory core header
│   ├── chip8.cpp                 # CPU opcode interpreter & savestate implementation
│   ├── app_state.h               # Application state machine, ROM items & config
│   ├── app_state.cpp             # Dynamic ROM discovery & persistent settings
│   ├── ui_font.h                 # 8x8 font system & geometric vector icons
│   ├── ui_font.cpp               # FONT_8X8 glyph table
│   ├── ui_renderer.h             # UI rendering engine declarations & hit-testing
│   ├── ui_renderer.cpp           # Launcher, gameplay, debugger & settings UI
│   └── main.cpp                  # Entry point, SDL2 loop, audio & input router
│
├── roms/                         # Game & Test ROM Collection
│   ├── games/                    # Classic games (Pong, Tetris, Blinky)
│   └── tests/                    # Timendus & Corax+ verification suite
│
├── tests/                        # Automated Unit Testing
│   └── test_core.cpp             # 23 headless unit tests for CPU core
│
├── assets/                       # Visual Assets
│   └── screenshots/              # Real high-resolution UI captures
│
├── docs/                         # Documentation
│   ├── ARCHITECTURE.md           # System architecture & technical specs
│   └── DEVELOPMENT.md            # Developer & contributor setup guide
│
├── .gitignore                    # Git ignore rules
├── Makefile                      # Cross-platform build script
├── README.md                     # Project presentation & showcase
├── LICENSE                       # MIT Open-Source License
├── CONTRIBUTING.md               # Contribution guidelines
└── CHANGELOG.md                  # Release version history
```

---

## 4. Testing & Verification

### Running Unit Tests
The unit test suite verifies CPU instruction semantics, arithmetic flags, stack balance, BCD math, and savestate round-tripping:
```bash
make test
# or:
.\run_tests.exe
```
Expected output:
```
23/23 checks passed
```

### Running Headless ROM Verification
Automated tests can run without initializing a window by rendering the display buffer directly to stdout:
```bash
# Timendus Logo Test:
./chip8 roms/tests/1-chip8-logo.ch8 --headless --cycles 1000 --dump-screen

# Corax+ Opcode Integrity Check:
./chip8 roms/tests/3-corax+.ch8 --headless --cycles 2000 --dump-screen

# Flags Arithmetic Test:
./chip8 roms/tests/4-flags.ch8 --headless --cycles 2000 --dump-screen
```

---

## 5. Adding New ROMs

1. Copy any standard `.ch8` binary into the `roms/` directory or subdirectories (e.g. `roms/games/`).
2. Start the application or click the **`RESCAN`** button in the launcher header.
3. The launcher will automatically detect the new file, generate a clean title from its filename, assign a category, create deterministic procedural pixel art, and make it available in the game grid immediately.

---

## 6. Adding Custom Color Themes

Color themes are defined in [`src/app_state.cpp`](file:///c:/Users/joshw/Downloads/chip8-emulator-main/src/app_state.cpp) within the `PALETTES` array:

```cpp
const Palette PALETTES[] = {
    // Name, Background RGB, Foreground RGB, Accent RGB
    {"Custom Synthwave", { 18,  12,  36}, {255, 110, 200}, {  0, 255, 230}},
    ...
};
```

Adding an entry automatically updates the theme selector in both the gameplay header and the Settings modal.
