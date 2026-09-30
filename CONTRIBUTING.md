# Contributing to CHIP//8 Arcade

Thank you for your interest in contributing to **CHIP//8 Arcade**! We welcome contributions ranging from bug fixes and code optimizations to documentation improvements and new feature suggestions.

---

## Code of Conduct

* Be respectful and considerate in discussions, issue threads, and pull requests.
* Focus on constructive, actionable feedback and collaborative problem-solving.

---

## Development Guidelines

1. **Language & Standard**:
   * All code must conform to standard **C++17**.
   * Use modern C++ features where appropriate (e.g. `std::clamp`, `std::filesystem`, structured bindings).
2. **Dependencies**:
   * Maintain zero external dependencies beyond standard C++ and **SDL2**. Do not introduce heavy third-party UI frameworks or dynamic font engines.
3. **Emulator Integrity**:
   * All 35 CHIP-8 opcodes must adhere strictly to standard CHIP-8 specifications.
   * Arithmetic operations must preserve register ordering rules (specifically `VF` flag ordering).
   * All 23 headless unit tests (`make test`) must continue to pass cleanly.
4. **Code Quality**:
   * Use descriptive variable and function names.
   * Add meaningful comments explaining complex virtual machine logic or coordinate mapping.

---

## Submitting a Pull Request

1. **Fork and Clone** the repository.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Build and Test**:
   Verify that the application compiles without warnings and all unit tests pass:
   ```bash
   make
   make test
   ```
4. **Commit with Clear Messages**:
   Follow conventional commit style (e.g. `feat: add custom scanline intensity slider`, `fix: address stack pointer decrement in 00EE`).
5. **Open a Pull Request** describing your changes and verification steps.

---

## Reporting Issues

When filing a bug report:
* Include your operating system, compiler version, and SDL2 version.
* Provide exact steps or ROM files needed to reproduce the issue.
* If reporting an opcode defect, reference the relevant opcode and expected vs. actual register values.
