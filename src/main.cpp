#define SDL_MAIN_HANDLED
#include "chip8.h"
#include "app_state.h"
#include "ui_renderer.h"
#include "ui_font.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <filesystem>
#include <algorithm>

// CHIP-8 16-key mapping to standard modern PC keyboard
// Keypad: 1 2 3 C / 4 5 6 D / 7 8 9 E / A 0 B F
static const SDL_Keycode KEYMAP[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E
    SDLK_v  // F
};

static std::string get_save_filename(int slot) {
    return "savestate_slot" + std::to_string(slot) + ".c8s";
}

static void apply_game_key(Chip8& chip8, AppContext& ctx, SDL_Keycode k, uint8_t state) {
    for (int i = 0; i < 16; i++) {
        if (k == KEYMAP[i]) chip8.key[i] = state;
    }

    RomItem* cur = ctx.get_selected_rom();
    std::string lower = cur ? cur->title : "";
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("pong") != std::string::npos) {
        if (k == SDLK_w) chip8.key[1] = state;
        if (k == SDLK_s) chip8.key[4] = state;
        if (k == SDLK_UP) chip8.key[12] = state;
        if (k == SDLK_DOWN) chip8.key[13] = state;
    } else if (lower.find("tetris") != std::string::npos) {
        if (k == SDLK_a || k == SDLK_LEFT) chip8.key[4] = state;
        if (k == SDLK_d || k == SDLK_RIGHT) chip8.key[6] = state;
        if (k == SDLK_w || k == SDLK_UP) chip8.key[5] = state;
        if (k == SDLK_s || k == SDLK_DOWN) chip8.key[7] = state;
    } else if (lower.find("blinky") != std::string::npos) {
        if (k == SDLK_w || k == SDLK_UP) chip8.key[3] = state;
        if (k == SDLK_s || k == SDLK_DOWN) chip8.key[6] = state;
        if (k == SDLK_a || k == SDLK_LEFT) chip8.key[7] = state;
        if (k == SDLK_d || k == SDLK_RIGHT) chip8.key[8] = state;
    } else {
        if (k == SDLK_w || k == SDLK_UP) chip8.key[5] = state;
        if (k == SDLK_s || k == SDLK_DOWN) chip8.key[8] = state;
        if (k == SDLK_a || k == SDLK_LEFT) chip8.key[7] = state;
        if (k == SDLK_d || k == SDLK_RIGHT) chip8.key[9] = state;
    }
}

static void audio_callback(void* userdata, uint8_t* stream, int len) {
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*)stream;
    int samples = len / 2;
    bool* beeping = (bool*)userdata;

    for (int i = 0; i < samples; i++) {
        if (*beeping) {
            // 440 Hz square wave at 44100 Hz sampling rate
            int16_t val = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = val;
        } else {
            audio_buffer[i] = 0;
            sample_index = 0;
        }
    }
}

static std::string disassemble_opcode_str(uint16_t op, uint16_t addr) {
    std::ostringstream oss;
    oss << "[0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(3) << addr << "] 0x"
        << std::setw(4) << op << "  ";

    uint8_t x = (op & 0x0F00) >> 8;
    uint8_t y = (op & 0x00F0) >> 4;
    uint8_t n = op & 0x000F;
    uint8_t kk = op & 0x00FF;
    uint16_t nnn = op & 0x0FFF;

    switch (op & 0xF000) {
        case 0x0000:
            if (op == 0x00E0) oss << "CLS";
            else if (op == 0x00EE) oss << "RET";
            else oss << "SYS 0x" << std::setw(3) << nnn;
            break;
        case 0x1000: oss << "JP 0x" << std::setw(3) << nnn; break;
        case 0x2000: oss << "CALL 0x" << std::setw(3) << nnn; break;
        case 0x3000: oss << "SE V" << (int)x << ", 0x" << std::setw(2) << (int)kk; break;
        case 0x4000: oss << "SNE V" << (int)x << ", 0x" << std::setw(2) << (int)kk; break;
        case 0x5000: oss << "SE V" << (int)x << ", V" << (int)y; break;
        case 0x6000: oss << "LD V" << (int)x << ", 0x" << std::setw(2) << (int)kk; break;
        case 0x7000: oss << "ADD V" << (int)x << ", 0x" << std::setw(2) << (int)kk; break;
        case 0x8000:
            switch (n) {
                case 0x0: oss << "LD V" << (int)x << ", V" << (int)y; break;
                case 0x1: oss << "OR V" << (int)x << ", V" << (int)y; break;
                case 0x2: oss << "AND V" << (int)x << ", V" << (int)y; break;
                case 0x3: oss << "XOR V" << (int)x << ", V" << (int)y; break;
                case 0x4: oss << "ADD V" << (int)x << ", V" << (int)y; break;
                case 0x5: oss << "SUB V" << (int)x << ", V" << (int)y; break;
                case 0x6: oss << "SHR V" << (int)x; break;
                case 0x7: oss << "SUBN V" << (int)x << ", V" << (int)y; break;
                case 0xE: oss << "SHL V" << (int)x; break;
                default: oss << "UNKNOWN"; break;
            }
            break;
        case 0x9000: oss << "SNE V" << (int)x << ", V" << (int)y; break;
        case 0xA000: oss << "LD I, 0x" << std::setw(3) << nnn; break;
        case 0xB000: oss << "JP V0, 0x" << std::setw(3) << nnn; break;
        case 0xC000: oss << "RND V" << (int)x << ", 0x" << std::setw(2) << (int)kk; break;
        case 0xD000: oss << "DRW V" << (int)x << ", V" << (int)y << ", " << (int)n; break;
        case 0xE000:
            if (kk == 0x9E) oss << "SKP V" << (int)x;
            else if (kk == 0xA1) oss << "SKNP V" << (int)x;
            else oss << "UNKNOWN";
            break;
        case 0xF000:
            switch (kk) {
                case 0x07: oss << "LD V" << (int)x << ", DT"; break;
                case 0x0A: oss << "LD V" << (int)x << ", K"; break;
                case 0x15: oss << "LD DT, V" << (int)x; break;
                case 0x18: oss << "LD ST, V" << (int)x; break;
                case 0x1E: oss << "ADD I, V" << (int)x; break;
                case 0x29: oss << "LD F, V" << (int)x; break;
                case 0x33: oss << "LD B, V" << (int)x; break;
                case 0x55: oss << "LD [I], V" << (int)x; break;
                case 0x65: oss << "LD V" << (int)x << ", [I]"; break;
                default: oss << "UNKNOWN"; break;
            }
            break;
        default: oss << "UNKNOWN"; break;
    }
    return oss.str();
}

static int disassemble_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return 1;
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::cout << "--- Disassembly: " << filename << " (" << buffer.size() << " bytes) ---" << std::endl;
    for (size_t i = 0; i + 1 < buffer.size(); i += 2) {
        uint16_t addr = 0x200 + (uint16_t)i;
        uint16_t op = (buffer[i] << 8) | buffer[i + 1];
        std::cout << disassemble_opcode_str(op, addr) << std::endl;
    }
    return 0;
}

static void dump_screen(const Chip8& chip8) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            std::cout << (chip8.display[x + y * 64] ? '#' : '.');
        }
        std::cout << "\n";
    }
    std::cout << std::flush;
}

int main(int argc, char** argv) {
    bool headless = false;
    bool dump = false;
    bool disasm_mode = false;
    bool demo_mode = false;
    int cycles = 1000;
    std::string cli_rom_path = "";

    // Parse CLI arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
        } else if (arg == "--dump-screen") {
            dump = true;
        } else if (arg == "--disasm") {
            disasm_mode = true;
        } else if (arg == "--demo") {
            demo_mode = true;
        } else if (arg == "--cycles" && i + 1 < argc) {
            cycles = std::stoi(argv[++i]);
        } else if (arg.rfind("--", 0) != 0) {
            cli_rom_path = arg;
        }
    }

    // CLI Disassembler Mode
    if (disasm_mode) {
        if (cli_rom_path.empty()) {
            std::cerr << "Usage: " << argv[0] << " --disasm <ROM file>" << std::endl;
            return 1;
        }
        return disassemble_file(cli_rom_path);
    }

    // CLI Headless Test Mode
    if (headless) {
        if (cli_rom_path.empty()) {
            std::cerr << "Headless mode requires a ROM path" << std::endl;
            return 1;
        }
        Chip8 chip8;
        if (!chip8.load_rom(cli_rom_path)) return 1;
        for (int i = 0; i < cycles; i++) {
            chip8.emulate_cycle();
            if (i % 10 == 0) chip8.update_timers();
        }
        if (dump) dump_screen(chip8);
        return 0;
    }

    // Initialize SDL2
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Audio Setup
    bool beeping = false;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &beeping;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device != 0) {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    const int WIN_WIDTH = 1280;
    const int WIN_HEIGHT = 720;

    SDL_Window* window = SDL_CreateWindow("CHIP//8 Arcade — Modern Retro Emulator",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    AppContext ctx;
    ctx.init();
    ctx.demo_mode = demo_mode;

    Chip8 chip8;
    UiRenderer ui_renderer;

    // Direct ROM launch from command line
    if (!cli_rom_path.empty()) {
        if (chip8.load_rom(cli_rom_path)) {
            ctx.screen = ScreenMode::GAMEPLAY;
            // Match with discovered ROMs or add as custom
            bool found = false;
            for (size_t i = 0; i < ctx.roms.size(); i++) {
                if (ctx.roms[i].path == cli_rom_path || ctx.roms[i].filename == cli_rom_path) {
                    ctx.selected_rom_index = (int)i;
                    ctx.record_played((int)i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                RomItem custom;
                custom.path = cli_rom_path;
                custom.filename = std::filesystem::path(cli_rom_path).filename().string();
                custom.title = std::filesystem::path(cli_rom_path).stem().string();
                custom.category = "Custom ROM";
                custom.description = "Directly loaded ROM binary.";
                ctx.roms.push_back(custom);
                ctx.selected_rom_index = (int)ctx.roms.size() - 1;
            }
            ctx.show_toast("Loaded ROM: " + ctx.roms[ctx.selected_rom_index].title);
        } else {
            ctx.show_toast("Failed to open ROM: " + cli_rom_path, 255, 60, 60);
        }
    }

    SDL_StartTextInput();

    const Uint32 FRAME_MS = 1000 / 60;
    bool running = true;

    while (running) {
        Uint32 frame_start = SDL_GetTicks();
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEMOTION) {
                ui_renderer.handle_mouse_move(ctx, event.motion.x, event.motion.y, WIN_WIDTH, WIN_HEIGHT);
            } else if (event.type == SDL_MOUSEWHEEL) {
                ui_renderer.handle_mouse_wheel(ctx, event.wheel.y, WIN_HEIGHT);
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                UiClickResult res = ui_renderer.handle_mouse_click(ctx, chip8, event.button.x, event.button.y, WIN_WIDTH, WIN_HEIGHT);

                switch (res.type) {
                    case UiClickResult::LAUNCH_ROM: {
                        if (res.rom_index >= 0 && res.rom_index < (int)ctx.roms.size()) {
                            ctx.selected_rom_index = res.rom_index;
                            const auto& rom = ctx.roms[res.rom_index];
                            chip8.reset();
                            if (chip8.load_rom(rom.path)) {
                                ctx.screen = ScreenMode::GAMEPLAY;
                                ctx.paused = false;
                                ctx.record_played(res.rom_index);
                                ctx.show_toast("Launching " + rom.title, 0, 255, 200);
                            } else {
                                ctx.show_toast("Failed to load ROM: " + rom.filename, 255, 60, 60);
                            }
                        }
                        break;
                    }
                    case UiClickResult::RETURN_TO_LAUNCHER:
                        ctx.screen = ScreenMode::LAUNCHER;
                        ctx.show_debugger = false;
                        ctx.show_settings_modal = false;
                        ctx.show_keypad_modal = false;
                        break;
                    case UiClickResult::TOGGLE_FAVORITE:
                        ctx.toggle_favorite(res.rom_index);
                        break;
                    case UiClickResult::SET_TAB:
                        ctx.tab = (TabFilter)res.int_val;
                        ctx.update_filter();
                        break;
                    case UiClickResult::SET_PALETTE:
                        ctx.config.palette = res.int_val;
                        ctx.config.save();
                        ctx.show_toast(std::string("Theme: ") + PALETTES[ctx.config.palette].name,
                                       PALETTES[ctx.config.palette].accent[0],
                                       PALETTES[ctx.config.palette].accent[1],
                                       PALETTES[ctx.config.palette].accent[2]);
                        break;
                    case UiClickResult::TOGGLE_PAUSE:
                        ctx.paused = !ctx.paused;
                        break;
                    case UiClickResult::STEP_INSTRUCTION:
                        if (ctx.paused) {
                            chip8.emulate_cycle();
                        }
                        break;
                    case UiClickResult::RESTART_ROM: {
                        RomItem* cur = ctx.get_selected_rom();
                        if (cur) {
                            chip8.reset();
                            chip8.load_rom(cur->path);
                            ctx.show_toast("Restarted " + cur->title, 255, 180, 50);
                        }
                        break;
                    }
                    case UiClickResult::SET_SLOT:
                        ctx.config.savestate_slot = res.int_val;
                        ctx.config.save();
                        ctx.show_toast("Active Slot: " + std::to_string(res.int_val), 255, 200, 50);
                        break;
                    case UiClickResult::SAVE_STATE: {
                        std::string fn = get_save_filename(ctx.config.savestate_slot);
                        if (chip8.save_state(fn)) {
                            ctx.show_toast("State saved to Slot " + std::to_string(ctx.config.savestate_slot), 50, 255, 150);
                        } else {
                            ctx.show_toast("Save FAILED (Slot " + std::to_string(ctx.config.savestate_slot) + ")", 255, 60, 60);
                        }
                        break;
                    }
                    case UiClickResult::LOAD_STATE: {
                        std::string fn = get_save_filename(ctx.config.savestate_slot);
                        if (chip8.load_state(fn)) {
                            ctx.show_toast("State loaded from Slot " + std::to_string(ctx.config.savestate_slot), 0, 220, 255);
                        } else {
                            ctx.show_toast("Load FAILED (Slot " + std::to_string(ctx.config.savestate_slot) + ")", 255, 60, 60);
                        }
                        break;
                    }
                    case UiClickResult::TOGGLE_DEBUGGER:
                        ctx.show_debugger = !ctx.show_debugger;
                        break;
                    case UiClickResult::TOGGLE_SETTINGS:
                        ctx.show_settings_modal = !ctx.show_settings_modal;
                        break;
                    case UiClickResult::TOGGLE_KEYPAD:
                        ctx.show_keypad_modal = !ctx.show_keypad_modal;
                        break;
                    case UiClickResult::TOGGLE_SCANLINES:
                        ctx.config.scanlines_enabled = !ctx.config.scanlines_enabled;
                        ctx.config.save();
                        break;
                    case UiClickResult::TOGGLE_MUTE:
                        ctx.config.sound_muted = !ctx.config.sound_muted;
                        ctx.config.save();
                        ctx.show_toast(ctx.config.sound_muted ? "Sound Muted" : "Sound Enabled", 0, 220, 255);
                        break;
                    case UiClickResult::SET_SPEED: {
                        int step = (ctx.config.cycles_per_frame < 20) ? 1 : 5;
                        ctx.config.cycles_per_frame = std::clamp(ctx.config.cycles_per_frame + res.int_val * step, 1, 100);
                        ctx.config.save();
                        break;
                    }
                    case UiClickResult::RESCAN_ROMS:
                        ctx.scan_roms("roms");
                        ctx.show_toast("Rescanned: " + std::to_string(ctx.roms.size()) + " ROMs found", 0, 255, 200);
                        break;
                    case UiClickResult::CLEAR_RECENT:
                        ctx.config.recent_roms.clear();
                        ctx.config.save();
                        ctx.update_filter();
                        ctx.show_toast("Recent list cleared", 200, 200, 200);
                        break;
                    default:
                        break;
                }
            } else if (event.type == SDL_TEXTINPUT && ctx.search_focused && ctx.screen == ScreenMode::LAUNCHER) {
                ctx.search_query += event.text.text;
                ctx.update_filter();
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode k = event.key.keysym.sym;

                // Search typing in launcher
                if (ctx.search_focused && ctx.screen == ScreenMode::LAUNCHER) {
                    if (k == SDLK_BACKSPACE && !ctx.search_query.empty()) {
                        ctx.search_query.pop_back();
                        ctx.update_filter();
                        continue;
                    } else if (k == SDLK_RETURN || k == SDLK_ESCAPE) {
                        ctx.search_focused = false;
                        continue;
                    }
                }

                // Global Shortcuts
                if (k == SDLK_ESCAPE || (k == SDLK_m && ctx.screen == ScreenMode::GAMEPLAY)) {
                    if (ctx.show_settings_modal) ctx.show_settings_modal = false;
                    else if (ctx.show_keypad_modal) ctx.show_keypad_modal = false;
                    else if (ctx.screen == ScreenMode::GAMEPLAY) {
                        ctx.screen = ScreenMode::LAUNCHER;
                        ctx.show_debugger = false;
                    } else {
                        running = false;
                    }
                } else if (ctx.screen == ScreenMode::LAUNCHER && !ctx.search_focused && (k >= SDLK_1 && k <= SDLK_9)) {
                    int idx = k - SDLK_1;
                    if (idx < (int)ctx.filtered_indices.size()) {
                        int r_idx = ctx.filtered_indices[idx];
                        ctx.selected_rom_index = r_idx;
                        const auto& rom = ctx.roms[r_idx];
                        chip8.reset();
                        if (chip8.load_rom(rom.path)) {
                            ctx.screen = ScreenMode::GAMEPLAY;
                            ctx.paused = false;
                            ctx.record_played(r_idx);
                            ctx.show_toast("Launching " + rom.title, 0, 255, 200);
                        }
                    }
                } else if (ctx.screen == ScreenMode::LAUNCHER && !ctx.search_focused && (k == SDLK_UP || k == SDLK_w)) {
                    if (!ctx.filtered_indices.empty()) {
                        auto it = std::find(ctx.filtered_indices.begin(), ctx.filtered_indices.end(), ctx.selected_rom_index);
                        if (it != ctx.filtered_indices.end() && it != ctx.filtered_indices.begin()) {
                            ctx.selected_rom_index = *(it - 1);
                        } else {
                            ctx.selected_rom_index = ctx.filtered_indices.front();
                        }
                    }
                } else if (ctx.screen == ScreenMode::LAUNCHER && !ctx.search_focused && (k == SDLK_DOWN || k == SDLK_s)) {
                    if (!ctx.filtered_indices.empty()) {
                        auto it = std::find(ctx.filtered_indices.begin(), ctx.filtered_indices.end(), ctx.selected_rom_index);
                        if (it != ctx.filtered_indices.end() && (it + 1) != ctx.filtered_indices.end()) {
                            ctx.selected_rom_index = *(it + 1);
                        } else {
                            ctx.selected_rom_index = ctx.filtered_indices.front();
                        }
                    }
                } else if (ctx.screen == ScreenMode::LAUNCHER && !ctx.search_focused && (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)) {
                    if (ctx.selected_rom_index >= 0 && ctx.selected_rom_index < (int)ctx.roms.size()) {
                        const auto& rom = ctx.roms[ctx.selected_rom_index];
                        chip8.reset();
                        if (chip8.load_rom(rom.path)) {
                            ctx.screen = ScreenMode::GAMEPLAY;
                            ctx.paused = false;
                            ctx.record_played(ctx.selected_rom_index);
                            ctx.show_toast("Launching " + rom.title, 0, 255, 200);
                        }
                    }
                } else if (k == SDLK_TAB || k == SDLK_c) {
                    ctx.config.palette = (ctx.config.palette + 1) % NUM_PALETTES;
                    ctx.config.save();
                    ctx.show_toast(std::string("Theme: ") + PALETTES[ctx.config.palette].name,
                                   PALETTES[ctx.config.palette].accent[0],
                                   PALETTES[ctx.config.palette].accent[1],
                                   PALETTES[ctx.config.palette].accent[2]);
                } else if (k == SDLK_h) {
                    if (ctx.screen == ScreenMode::GAMEPLAY) {
                        ctx.show_debugger = !ctx.show_debugger;
                    }
                } else if (k == SDLK_SPACE || k == SDLK_p) {
                    if (ctx.screen == ScreenMode::GAMEPLAY) {
                        ctx.paused = !ctx.paused;
                    }
                } else if (k == SDLK_n) {
                    if (ctx.screen == ScreenMode::GAMEPLAY && ctx.paused) {
                        chip8.emulate_cycle();
                    }
                } else if ((k == SDLK_r) && (event.key.keysym.mod & KMOD_CTRL)) {
                    if (ctx.screen == ScreenMode::GAMEPLAY) {
                        RomItem* cur = ctx.get_selected_rom();
                        if (cur) {
                            chip8.reset();
                            chip8.load_rom(cur->path);
                            ctx.show_toast("Restarted " + cur->title, 255, 180, 50);
                        }
                    }
                } else if (k == SDLK_UP || k == SDLK_EQUALS || k == SDLK_PLUS) {
                    int step = (ctx.config.cycles_per_frame < 20) ? 1 : 5;
                    ctx.config.cycles_per_frame = std::clamp(ctx.config.cycles_per_frame + step, 1, 100);
                    ctx.config.save();
                } else if (k == SDLK_DOWN || k == SDLK_MINUS) {
                    int step = (ctx.config.cycles_per_frame < 20) ? 1 : 5;
                    ctx.config.cycles_per_frame = std::clamp(ctx.config.cycles_per_frame - step, 1, 100);
                    ctx.config.save();
                } else if (k == SDLK_BACKSPACE) {
                    ctx.config.cycles_per_frame = 10;
                    ctx.config.save();
                    ctx.show_toast("Speed reset: 10 cyc/f (600 Hz)", 0, 240, 255);
                } else if (k == SDLK_F1) { ctx.config.savestate_slot = 1; ctx.show_toast("Slot 1 Selected", 255, 200, 50); }
                else if (k == SDLK_F2) { ctx.config.savestate_slot = 2; ctx.show_toast("Slot 2 Selected", 255, 200, 50); }
                else if (k == SDLK_F3) { ctx.config.savestate_slot = 3; ctx.show_toast("Slot 3 Selected", 255, 200, 50); }
                else if (k == SDLK_F4) { ctx.config.savestate_slot = 4; ctx.show_toast("Slot 4 Selected", 255, 200, 50); }
                else if (k == SDLK_F5) {
                    std::string fn = get_save_filename(ctx.config.savestate_slot);
                    if (chip8.save_state(fn)) {
                        ctx.show_toast("State saved to Slot " + std::to_string(ctx.config.savestate_slot), 50, 255, 150);
                    } else {
                        ctx.show_toast("Save FAILED (Slot " + std::to_string(ctx.config.savestate_slot) + ")", 255, 60, 60);
                    }
                } else if (k == SDLK_F9) {
                    std::string fn = get_save_filename(ctx.config.savestate_slot);
                    if (chip8.load_state(fn)) {
                        ctx.show_toast("State loaded from Slot " + std::to_string(ctx.config.savestate_slot), 0, 220, 255);
                    } else {
                        ctx.show_toast("Load FAILED (Slot " + std::to_string(ctx.config.savestate_slot) + ")", 255, 60, 60);
                    }
                }

                // Forward active keys to CHIP-8 keypad during gameplay
                if (ctx.screen == ScreenMode::GAMEPLAY && !ctx.show_settings_modal && !ctx.show_keypad_modal) {
                    apply_game_key(chip8, ctx, k, 1);
                }
            } else if (event.type == SDL_KEYUP) {
                if (ctx.screen == ScreenMode::GAMEPLAY) {
                    apply_game_key(chip8, ctx, event.key.keysym.sym, 0);
                }
            }
        }

        // Emulation step
        if (ctx.screen == ScreenMode::GAMEPLAY && !ctx.paused && !ctx.show_settings_modal && !ctx.show_keypad_modal) {
            for (int i = 0; i < ctx.config.cycles_per_frame; i++) {
                chip8.emulate_cycle();
            }
            chip8.update_timers(); // 60 Hz timer tick
        }

        // Sound state
        beeping = (!ctx.config.sound_muted && ctx.screen == ScreenMode::GAMEPLAY && !ctx.paused && chip8.get_sound_timer() > 0);

        // Render Frame
        ui_renderer.render(renderer, ctx, chip8, WIN_WIDTH, WIN_HEIGHT);

        // Frame rate limiter (60 FPS)
        Uint32 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < FRAME_MS) {
            SDL_Delay(FRAME_MS - elapsed);
        }
    }

    SDL_StopTextInput();
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}