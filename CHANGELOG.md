# Changelog

All notable changes to **CHIP//8 Arcade** are documented in this file in reverse chronological order.

---

## [1.0.0] - 2026-09-30 — *CHIP//8 Arcade Release*

### Added
* **Interactive ROM Library Launcher (1280×720)**:
  * Automatic recursive `.ch8` file discovery from the `roms/` hierarchy.
  * Deterministic procedural pixel artwork generator customized for each game (Pong, Tetris, Blinky, Test Suite).
  * Filter tabs: `ALL ROMS`, `FAVORITES`, `RECENT`, `GAMES`, `TESTS`.
  * Live search input with real-time title and category matching.
  * Persistent favorites list and recently played history.
* **Gameplay & Emulation Enhancements**:
  * Centered $64 \times 32$ integer-scaled display with arcade bezel and subtle CRT scanlines toggle.
  * Real-time CPU execution speed slider (1 to 100 cycles/frame, ~60 Hz to ~6,000 Hz) with immediate runtime effect.
  * 6 authentic retro color themes: Classic Green, Amber CRT, Neon Cyberpunk, White Arcade, Game Boy 1989, and Solarized Ruby.
  * Audio synthesis with 440 Hz square wave beep and sound mute toggle.
* **Multi-Slot Savestate Engine**:
  * 4 binary savestate slots with magic header `CH8S` and format version verification.
  * Quick save (<kbd>F5</kbd>) and quick load (<kbd>F9</kbd>) hotkeys and dedicated UI buttons.
* **Integrated Real-State CPU Debugger**:
  * Live sidebar (<kbd>H</kbd>) inspecting real registers V0 through VF in hex.
  * Real-time display of PC, Index register I, Stack Pointer SP, call stack, and timers DT/ST.
  * Opcode disassembler translating binary instructions into readable mnemonics.
  * Memory inspection window displaying 8 bytes surrounding the active PC.
  * Single-step execution (<kbd>N</kbd>) while paused.
* **Settings & Controls Modals**:
  * Visual 16-key CHIP-8 hex keypad mapping diagram.
  * Options for CRT scanlines, audio muting, and directory rescanning.
* **Documentation & Asset Suite**:
  * Comprehensive `docs/ARCHITECTURE.md` and `docs/DEVELOPMENT.md`.
  * Real application screenshot gallery in `assets/screenshots/`.

---

## [0.5.0] - 2026-09-30 — *Defect Remediation & Core Verification*

### Fixed
* **Renderer Vertical Inversion**: Corrected pixel rendering calculation from `(31 - y) * scale` to `y * scale`.
* **Stack Pointer Off-by-One (`00EE` & `2nnn`)**: Fixed `00EE` to decrement `SP` before reading `stack[sp]`, with overflow and underflow bounds guards.
* **Arithmetic Flag Destruction (`VF`)**: Restructured all 8-series opcodes (`8xy4`, `8xy5`, `8xy6`, `8xy7`, `8xyE`) to compute results using temporaries and assign `v[0xF]` last.
* **Borrow Flag Logic (`8xy5` & `8xy7`)**: Corrected condition from strict `>` to `>=` so identical operands correctly yield `VF = 1`.
* **Non-Blocking Key Wait (`Fx0A`)**: Fixed `Fx0A` to halt instruction advance until an active keypress is detected.
* **BCD Conversion Math (`Fx33`)**: Corrected tens digit calculation using `(value / 10) % 10`.
* **Memory Load/Store Bounds (`Fx55` & `Fx65`)**: Changed loop termination from `i < x` to `i <= x` to include register $V_x$ inclusively.
* **Timer & Frame Pacing Decoupling**: Extracted timer updates to 60 Hz tick function independent of CPU instruction cycle counts.

### Added
* Built-in 23-test headless verification suite (`make test`).
* Headless execution flag (`--headless`) and ASCII console display dump (`--dump-screen`).
* Standalone ROM disassembler flag (`--disasm`).

---

## [0.1.0] - 2026-09-28 — *Initial Prototype*

### Added
* Baseline CHIP-8 virtual machine structure with SDL2 rendering.
