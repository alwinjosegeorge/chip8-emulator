# CHIP-8 Emulator — 7-Minute Demo Recording Script

This script provides a structured, time-stamped walkthrough for recording a **7-minute demo video** showcasing the restored, bug-fixed, and feature-enhanced CHIP-8 emulator.

---

## Recording Setup & Tips

### OBS Studio Configuration
- **Base / Canvas Resolution**: 1920×1080 (1080p).
- **Framerate**: 30 FPS or 60 FPS.
- **Video Source**: **Window Capture** targeting the `Chip-8 Emulator` window (or Full Display Capture if showing terminal alongside).
- **Audio Capture**: Add **Desktop Audio** or **Application Audio Output Capture** to ensure the 440 Hz audio beep tone is clearly heard when games play sounds.
- **Emulator Command for Recording**:
  ```bash
  ./chip8 roms/Pong.ch8 --demo
  ```
  *(Launches with 20× scaling at 1280×640, HUD active, and the 5-second controls cheat-sheet banner).*

---

## Timed Recording Script (0:00 – 7:00)

```
================================================================================
 TIMELINE BREAKDOWN
 0:00 - 0:45 : Intro & Problem Statement
 0:45 - 2:30 : Bugs Found & Deep-Dive into Fixes
 2:30 - 3:30 : Headless Verification & Timendus Test Suite
 3:30 - 4:30 : Speed Control & Color Schemes
 4:30 - 5:30 : Savestate Slots (F1–F4, F5 Save, F9 Load, Error Handling)
 5:30 - 6:30 : Disassembler, Single-Step Debugger & ROM Browser
 6:30 - 7:00 : Wrap-up & Conclusion
================================================================================
```

---

### [0:00 – 0:45] Intro & Problem Statement

* **Visual**: Show the terminal and launch the emulator in demo mode: `./chip8 roms/Pong.ch8 --demo`. Show the initial controls cheat-sheet overlay.
* **Spoken Script**:
  > "Hello everyone! Welcome to this demonstration of our CHIP-8 emulator project. 
  > When we received this codebase, it was in a heavily broken state with intentional defects planted across CPU opcode decoding, arithmetic flag handling, memory bounds, graphics rendering, timer synchronization, and frame timing.
  > Today, we'll walk through how we audited and diagnosed every defect, brought the emulator to 100% compliance with standard CHIP-8 specifications, verified it against test suites and classic games, and implemented a suite of recording and player features including a built-in HUD, configurable palettes, multi-slot binary savestates, and an integrated disassembler and step debugger."

---

### [0:45 – 2:30] Bugs Found & Deep-Dive into Fixes

* **Visual**: Show code snippets in `src/chip8.cpp` and `src/main.cpp` or run games highlighting the fixed behavior.
* **Spoken Script**:
  > "Let's walk through the major bugs we found and how we resolved each one:
  > 
  > 1. **Renderer Vertical Inversion (Y-Flip)**:
  >    In `main.cpp`, pixels were drawn using `(31 - y) * SCALE`, causing every sprite to render upside-down. We corrected this to `y * SCALE`.
  > 
  > 2. **Stack Pointer Underflow & Off-by-One (`00EE` & `2nnn`)**:
  >    In opcode `2nnn` (subroutine call), the return address was pushed and `SP` incremented. However, in `00EE` (return), `SP` was read before decrementing, reading garbage from the uninitialized stack top. We decremented `SP` first, retrieved the return address, and added bounds guards against underflow and overflow.
  > 
  > 3. **Arithmetic Flag Ordering & VF Overwrite (`8xy4`, `8xy5`, `8xy6`, `8xy7`, `8xyE`)**:
  >    In arithmetic operations, the carry/borrow flag was set into `VF` before writing the arithmetic result into `Vx`. When `Vx` was register `VF`, the result immediately destroyed the flag! We restructured all 8-series opcodes to compute via temporary variables, store the result into `Vx`, and assign `VF` last.
  > 
  > 4. **Borrow Flag Logic (`8xy5` & `8xy7`)**:
  >    The original code used strict inequality `>` instead of `>=`. In standard CHIP-8, `VF = 1` indicates NOT borrow (i.e. $Vx \ge Vy$). We fixed this so $Vx == Vy$ correctly yields `VF = 1`.
  > 
  > 5. **Non-Blocking Key Wait (`Fx0A`)**:
  >    `Fx0A` was advancing `PC += 2` unconditionally even when no key was pressed. We fixed it to halt/repeat the instruction until an active keypress is detected.
  > 
  > 6. **Binary-Coded Decimal Math (`Fx33`)**:
  >    The tens digit was computed as `value / 10`, which produced two-digit values for numbers over 99. We corrected it to `(value / 10) % 10`.
  > 
  > 7. **Memory Load/Store Bounds (`Fx55` & `Fx65`)**:
  >    The loops checked `i < x`, omitting the $x$-th register. We corrected the condition to `i <= x` to include $V_x$ inclusively.
  > 
  > 8. **Timer & Frame Timing Decoupling**:
  >    Timers were being decremented per CPU instruction cycle. We extracted them into `update_timers()` called at exactly 60 Hz, and eliminated the blocking `SDL_Delay(16)` inside the cycle loop so CPU cycles run at proper speed with frame-rate budget pacing."

---

### [2:30 – 3:30] Headless Test Mode & Timendus Test Suite

* **Visual**: Switch to terminal. Run `make test`, then run the Timendus test ROMs headless with `--dump-screen`.
* **Commands to run**:
  ```bash
  make test
  ./chip8 roms/tests/1-chip8-logo.ch8 --headless --cycles 1000 --dump-screen
  ./chip8 roms/tests/2-ibm-logo.ch8 --headless --cycles 1000 --dump-screen
  ./chip8 roms/tests/3-corax+.ch8 --headless --cycles 2000 --dump-screen
  ./chip8 roms/tests/4-flags.ch8 --headless --cycles 2000 --dump-screen
  ```
* **Spoken Script**:
  > "To ensure rigorous verification without relying solely on manual observation, we implemented two automated testing mechanisms:
  > First, running `make test` executes our built-in C++ test suite covering stack balance, arithmetic carry, borrow, shifts, BCD conversion, register dumps, and savestate serialization. All 23 tests pass cleanly.
  > Second, we built a headless testing mode (`--headless --cycles N --dump-screen`) that runs without initializing SDL and renders the frame buffer directly to the console as ASCII.
  > Look at these test results: Timendus 1-chip8-logo renders the full logo, 2-ibm-logo outputs the clean IBM emblem, and the Corax+ opcode and Flags verification tests complete with all checkmarks passing!"

---

### [3:30 – 4:30] Speed Control & Color Palettes

* **Visual**: Launch `./chip8 roms/Pong.ch8 --demo`. Press `+` / `-` (or `Up` / `Down`), then press `C` to cycle palettes. Toggle HUD with `H`.
* **Spoken Script**:
  > "Now let's launch a game in interactive demo mode using `./chip8 roms/Pong.ch8 --demo`.
  > Notice the on-screen HUD in the top corner, rendered using our built-in 3x5 font without any external font libraries. Pressing `H` toggles this HUD overlay at any time.
  > 
  > Using the `+` and `-` keys (or `Up`/`Down` arrows), we can adjust CPU cycles per frame in real-time, ranging from 1 cycle per frame for slow-motion inspection up to 200 cycles per frame for lightning speed. Pressing `Backspace` instantly resets to the standard 10 cycles/frame.
  > 
  > Pressing `C` or `Tab` cycles through four distinct retro color palettes:
  > - **Classic Green** phosphor
  > - **Amber CRT**
  > - **Neon High-Contrast** cyan
  > - **White-on-Black** monochrome
  > Both the HUD and window title update instantaneously to reflect the active settings."

---

### [4:30 – 5:30] Savestate Slots & Robust Serialization

* **Visual**: Play a few seconds of `Pong.ch8` or `Tetris.ch8`. Select Slot 1 (`F1`), save (`F5`). Let the game progress or lose, then press `F9` to restore. Switch to Slot 2 (`F2`), save, switch back to Slot 1, and restore.
* **Spoken Script**:
  > "Next, let's look at the savestate engine.
  > We support multiple savestate slots using keys `F1` through `F4`.
  > When we press `F5`, the entire emulator state—memory, registers V0 through VF, index register, program counter, call stack, delay and sound timers, keypad state, and display buffer—is serialized into a binary format with a four-byte magic header `CH8S` and a format version tag.
  > 
  > Let's press `F5` to save in Slot 1. Notice the confirmation in the HUD. Now let the ball score or pieces fall... and press `F9`. Instantly, the game state, ball velocity, score, and screen are seamlessly restored!
  > If a state file is missing, corrupted, or has an incompatible version, the loader rejects it safely without modifying the running game or crashing."

---

### [5:30 – 6:30] Disassembler, Single-Step Debugger & ROM Browser

* **Visual**: 
  1. In terminal, run `./chip8 --disasm roms/Pong.ch8 | head -n 30`
  2. In the emulator, press `P` or `Space` to pause, then press `N` multiple times to step instruction by instruction.
  3. Run `./chip8` with no arguments to show the interactive ROM browser.
* **Spoken Script**:
  > "For debugging and development, we included three power tools:
  > 
  > First, the `--disasm` CLI flag disassembles any CHIP-8 binary into clean, readable assembly mnemonics alongside hex addresses and raw opcodes.
  > 
  > Second, while in the emulator, pressing `P` or `Space` pauses execution. While paused, pressing `N` single-steps the CPU exactly one instruction at a time. The HUD and terminal immediately display the current address, opcode, and decoded mnemonic as it executes.
  > 
  > Third, if you run the emulator without passing a ROM name, or pass a directory, it automatically scans for `.ch8` files and opens an interactive menu letting you pick which game to boot."

---

### [6:30 – 7:00] Wrap-Up & Conclusion

* **Visual**: Show `Pong.ch8` or `Tetris.ch8` running smoothly with sound beeps playing on paddle hits. Show terminal showing clean `make` and `git log`.
* **Spoken Script**:
  > "To summarize: We have eliminated every intentional bug, ensured 100% specification compliance verified by automated unit tests and the Timendus suite, and enhanced the emulator with dynamic speed controls, custom palettes, multi-slot binary savestates, a built-in 3x5 font HUD, and disassembler tools.
  > The entire project builds with a simple `make` using only standard C++17 and SDL2.
  > Thank you for watching!"

---

## Quick Reference: Hotkey Cheat-Sheet for Recording

| Key | Function | Visual Effect |
| :--- | :--- | :--- |
| `H` | Toggle HUD | Shows/hides overlay with ROM, speed, palette, slot, last opcode |
| `+` / `-` | Speed Control | Modifies cycles/frame (1 – 200) |
| `Backspace` | Reset Speed | Sets speed back to 10 cyc/frame |
| `C` / `Tab` | Cycle Palette | Switches between Green, Amber, Neon, and Monochrome |
| `Space` / `P` | Pause / Resume | Freezes emulation |
| `N` | Step Instruction | Advances 1 cycle while paused (prints mnemonic) |
| `F1` – `F4` | Select Slot | Selects Savestate Slot 1, 2, 3, or 4 |
| `F5` | Save State | Writes binary state to `savestate_slotN.c8s` |
| `F9` | Load State | Restores binary state from active slot |
| `ESC` | Quit | Cleanly closes emulator |
