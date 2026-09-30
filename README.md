# Chip-8 Emulator

A Chip-8 emulator built in C++ with SDL2 graphics and audio support.

> **Note to Participants:** 
> This codebase is intentionally incomplete and contains implementation defects across opcode handling, memory management, timing control, and rendering pipeline. Please consult the **Problem Statement** document for your exact submission guidelines and evaluation criteria.

## Features

- All 35 Chip-8 opcodes implemented
- 64x32 pixel display with SDL2 rendering
- Keyboard input support
- Sound effects (beep tone)
- 60 FPS rendering

## Architecture

Chip-8 is a virtual machine from the 1970s designed to make programming video games easier on early microcomputers.

### System Specifications

- **Memory**: 4 KB (4096 bytes)
  - `0x000-0x1FF`: Reserved for interpreter and fonts
  - `0x200-0xFFF`: Program/ROM space
- **Registers**:
  - 16 8-bit general-purpose registers (V0-VF)
  - VF doubles as a flag register for arithmetic operations
  - 16-bit index register (I)
  - 16-bit program counter (PC)
  - 8-bit stack pointer (SP)
- **Display**: 64x32 pixels, monochrome
- **Timers**: 
  - Delay timer (counts down at 60 Hz)
  - Sound timer (beeps when > 0, counts down at 60 Hz)
- **Stack**: 16 levels for subroutine calls
- **Keypad**: 16-key hexadecimal input

## Dependencies

- SDL2 library

### Installation

**Arch Linux:**
```bash
sudo pacman -S sdl2
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**macOS:**
```bash
brew install sdl2
```

## Building

1. Clone the repo
```bash
git clone https://github.com/TatHack-Tathva/chip8-emulator.git
```

2. Make it
```bash
make
```

## Usage

```bash
# Launch interactive graphical Home Page & Game Hub:
./chip8
# or with demo recording mode (20x scale, HUD ready):
./chip8 --demo

# Run directly with a specific ROM:
./chip8 roms/Pong.ch8
./chip8 roms/Tetris.ch8
./chip8 roms/Blinky.ch8

# Run with custom window scale:
./chip8 roms/Tetris.ch8 --scale 15

# Disassemble a ROM file:
./chip8 --disasm roms/Pong.ch8

# Run headless test with ASCII screen dump:
./chip8 roms/tests/1-chip8-logo.ch8 --headless --cycles 1000 --dump-screen
```

### Command-Line Flags

| Flag | Argument | Description |
| :--- | :--- | :--- |
| `--demo` | None | Enables demo/recording mode: HUD on, 20× scale (1280×640), 5-second controls cheat-sheet |
| `--scale` | `N` | Sets custom window scaling factor ($64N \times 32N$ pixels) |
| `--headless` | None | Runs without SDL window/audio initialization (pure CPU emulation) |
| `--cycles` | `N` | Number of CPU cycles to execute in headless mode (default: 1000) |
| `--dump-screen`| None | Prints the 64×32 display buffer as ASCII text upon completion |
| `--disasm` | None | Disassembles the specified ROM into hex addresses, opcodes, and mnemonics |

## Controls

### Home Page / Game Hub
When launched with `./chip8` or `./chip8 --demo`, an interactive GUI Home Screen is presented:
- **`Up` / `Down` Arrow** or **`W` / `S`**: Navigate game selection
- **`Enter` / `Space` / Left Mouse Click**: Launch selected game
- **`1` – `9`**: Quick-launch game by number
- **`C` / `Tab`**: Cycle color palettes live on menu
- **`ESC`**: Exit application

### In-Game Navigation & Hotkeys

| Key / Shortcut | Action | Description |
| :--- | :--- | :--- |
| `ESC` or `M` | **Return to Home** | Pauses gameplay and returns to the Home Page to choose another game |
| `H` | Toggle HUD | Shows/hides on-screen overlay (ROM, speed, palette, slot, last opcode) |
| `+` / `=` | Speed Up | Increases CPU cycles executed per frame (1..200) |
| `-` | Speed Down | Decreases CPU cycles executed per frame |
| `Backspace` | Reset Speed | Restores default emulation speed (10 cycles/frame, 600 Hz) |
| `C` / `Tab` | Cycle Palette | Modern Slate (White/Black), Classic Green, Amber CRT, Cyber Neon, Pure Monochrome |
| `Space` / `P` | Pause / Resume | Freezes/unfreezes CPU and timer execution |
| `N` | Single Step | While paused, executes exactly 1 cycle and prints instruction |
| `F1` – `F4` | Select Slot | Selects Savestate Slot 1, 2, 3, or 4 |
| `F5` | Quick Save | Saves state to active slot (`savestate_slotN.c8s`) |
| `F9` | Quick Load | Restores state from active slot (`savestate_slotN.c8s`) |

### Game Controls (WASD & Arrow Keys)

- **PONG**:
  - **Player 1 (Left)**: **`W`** (Up), **`S`** (Down) *(or retro `1`, `Q`)*
  - **Player 2 (Right)**: **`Up Arrow`** (Up), **`Down Arrow`** (Down) *(or retro `4`, `R`)*
- **TETRIS**:
  - Move Left / Right: **`A`** / **`D`** or **`Left`** / **`Right Arrow`**
  - Rotate: **`W`** or **`Up Arrow`**
  - Soft Drop: **`S`** or **`Down Arrow`**
- **BLINKY (Pac-Man)**:
  - 4-Way Movement: **`W`**, **`A`**, **`S`**, **`D`** or **Arrow Keys** (`Up`, `Left`, `Down`, `Right`)

### Standard 16-Key Hex Keypad Mapping (Fallback)

The original 16-key hex keypad remains fully supported on standard QWERTY:

```
Chip-8 Keypad:          QWERTY Keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│C│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│D│               │Q│W│E│R│
├─┼─┼─┼─┤      =        ├─┼─┼─┼─┤
│7│8│9│E│               │A│S│D│F│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│A│0│B│F│               │Z│X│C│V│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

## Testing & Headless Mode

Run headless tests without launching SDL:
```bash
./chip8 roms/tests/1-chip8-logo.ch8 --headless --cycles 1000 --dump-screen
```

Run the automated CPU core unit test suite:
```bash
make test
```

## Implementation Details

### Instruction Set

The emulator implements all 35 Chip-8 instructions, including:
- **Arithmetic**: ADD, SUB, AND, OR, XOR, shift operations
- **Graphics**: Draw sprites with XOR mode, collision detection
- **Flow control**: Jump, call/return subroutines, conditional skips
- **Memory**: Load/store registers, BCD conversion
- **Timers**: Delay and sound timer operations
- **Input**: Key press detection (blocking and non-blocking)

### Display

Graphics are rendered using SDL2:
- Each Chip-8 pixel is scaled 10× for visibility (640×320 window)
- XOR-based sprite drawing for collision detection
- 60 FPS rendering

### Audio

Simple square wave generation at 440 Hz (musical note A) plays when `sound_timer > 0`.

## Resources

- [Chip-8 ROMs Archive](https://github.com/kripod/chip8-roms)
