#define SDL_MAIN_HANDLED
#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>

const int DEFAULT_SCALE = 10;

// Keyboard mapping
SDL_Keycode keymap[16] = {
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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

// Speed control (CPU cycles executed per 60 Hz frame)
const int DEFAULT_CYCLES = 10;
const int MIN_CYCLES = 1;
const int MAX_CYCLES = 200;

// Colour palettes: {name, background {R,G,B}, foreground {R,G,B}}
struct Palette {
    const char* name;
    uint8_t bg[3];
    uint8_t fg[3];
};

const Palette PALETTES[] = {
    {"Modern Slate (White/Black)", { 18,  20,  24}, {245, 245, 250}}, // Default sleek dark-slate & white
    {"Classic Green CRT",          {  0,  20,   0}, { 51, 255,  51}},
    {"Amber Phosphor",             { 20,  10,   0}, {255, 176,   0}},
    {"Cyber Neon",                 {  5,   0,  20}, {  0, 255, 255}},
    {"Pure Monochrome",            {  0,   0,   0}, {255, 255, 255}},
};
const int NUM_PALETTES = sizeof(PALETTES) / sizeof(PALETTES[0]);

struct App {
    int cycles_per_frame = DEFAULT_CYCLES;
    int palette = 0;
    int slot = 1;
    int scale = DEFAULT_SCALE;
    bool paused = false;
    bool show_hud = false;
    bool demo_mode = false;
    bool in_menu = false;
    int menu_selection = 0;
    Uint32 start_time = 0;
    std::string rom_name;
    std::string status;
};

// 3x5 font: 5 rows per glyph, 3 bits wide (bit 2=left, bit 1=mid, bit 0=right)
const uint8_t* get_font3x5_glyph(char c){
    static const uint8_t GLYPH_UNKNOWN[5] = {0b111, 0b101, 0b101, 0b101, 0b111};
    static const uint8_t GLYPH_SPACE[5]   = {0, 0, 0, 0, 0};

    if(c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');

    switch(c){
        case ' ': return GLYPH_SPACE;
        case '0': { static const uint8_t g[5] = {0b111, 0b101, 0b101, 0b101, 0b111}; return g; }
        case '1': { static const uint8_t g[5] = {0b010, 0b110, 0b010, 0b010, 0b111}; return g; }
        case '2': { static const uint8_t g[5] = {0b111, 0b001, 0b111, 0b100, 0b111}; return g; }
        case '3': { static const uint8_t g[5] = {0b111, 0b001, 0b111, 0b001, 0b111}; return g; }
        case '4': { static const uint8_t g[5] = {0b101, 0b101, 0b111, 0b001, 0b001}; return g; }
        case '5': { static const uint8_t g[5] = {0b111, 0b100, 0b111, 0b001, 0b111}; return g; }
        case '6': { static const uint8_t g[5] = {0b111, 0b100, 0b111, 0b101, 0b111}; return g; }
        case '7': { static const uint8_t g[5] = {0b111, 0b001, 0b010, 0b010, 0b010}; return g; }
        case '8': { static const uint8_t g[5] = {0b111, 0b101, 0b111, 0b101, 0b111}; return g; }
        case '9': { static const uint8_t g[5] = {0b111, 0b101, 0b111, 0b001, 0b111}; return g; }

        case 'A': { static const uint8_t g[5] = {0b010, 0b101, 0b111, 0b101, 0b101}; return g; }
        case 'B': { static const uint8_t g[5] = {0b110, 0b101, 0b110, 0b101, 0b110}; return g; }
        case 'C': { static const uint8_t g[5] = {0b111, 0b100, 0b100, 0b100, 0b111}; return g; }
        case 'D': { static const uint8_t g[5] = {0b110, 0b101, 0b101, 0b101, 0b110}; return g; }
        case 'E': { static const uint8_t g[5] = {0b111, 0b100, 0b110, 0b100, 0b111}; return g; }
        case 'F': { static const uint8_t g[5] = {0b111, 0b100, 0b110, 0b100, 0b100}; return g; }
        case 'G': { static const uint8_t g[5] = {0b111, 0b100, 0b101, 0b101, 0b111}; return g; }
        case 'H': { static const uint8_t g[5] = {0b101, 0b101, 0b111, 0b101, 0b101}; return g; }
        case 'I': { static const uint8_t g[5] = {0b111, 0b010, 0b010, 0b010, 0b111}; return g; }
        case 'J': { static const uint8_t g[5] = {0b001, 0b001, 0b001, 0b101, 0b010}; return g; }
        case 'K': { static const uint8_t g[5] = {0b101, 0b110, 0b100, 0b110, 0b101}; return g; }
        case 'L': { static const uint8_t g[5] = {0b100, 0b100, 0b100, 0b100, 0b111}; return g; }
        case 'M': { static const uint8_t g[5] = {0b101, 0b111, 0b101, 0b101, 0b101}; return g; }
        case 'N': { static const uint8_t g[5] = {0b110, 0b101, 0b101, 0b101, 0b101}; return g; }
        case 'O': { static const uint8_t g[5] = {0b111, 0b101, 0b101, 0b101, 0b111}; return g; }
        case 'P': { static const uint8_t g[5] = {0b111, 0b101, 0b111, 0b100, 0b100}; return g; }
        case 'Q': { static const uint8_t g[5] = {0b111, 0b101, 0b101, 0b111, 0b001}; return g; }
        case 'R': { static const uint8_t g[5] = {0b110, 0b101, 0b110, 0b101, 0b101}; return g; }
        case 'S': { static const uint8_t g[5] = {0b111, 0b100, 0b111, 0b001, 0b111}; return g; }
        case 'T': { static const uint8_t g[5] = {0b111, 0b010, 0b010, 0b010, 0b010}; return g; }
        case 'U': { static const uint8_t g[5] = {0b101, 0b101, 0b101, 0b101, 0b111}; return g; }
        case 'V': { static const uint8_t g[5] = {0b101, 0b101, 0b101, 0b101, 0b010}; return g; }
        case 'W': { static const uint8_t g[5] = {0b101, 0b101, 0b101, 0b111, 0b101}; return g; }
        case 'X': { static const uint8_t g[5] = {0b101, 0b101, 0b010, 0b101, 0b101}; return g; }
        case 'Y': { static const uint8_t g[5] = {0b101, 0b101, 0b010, 0b010, 0b010}; return g; }
        case 'Z': { static const uint8_t g[5] = {0b111, 0b001, 0b010, 0b100, 0b111}; return g; }

        case ':': { static const uint8_t g[5] = {0b000, 0b010, 0b000, 0b010, 0b000}; return g; }
        case '-': { static const uint8_t g[5] = {0b000, 0b000, 0b111, 0b000, 0b000}; return g; }
        case '+': { static const uint8_t g[5] = {0b000, 0b010, 0b111, 0b010, 0b000}; return g; }
        case '=': { static const uint8_t g[5] = {0b000, 0b111, 0b000, 0b111, 0b000}; return g; }
        case '/': { static const uint8_t g[5] = {0b001, 0b001, 0b010, 0b100, 0b100}; return g; }
        case '[': { static const uint8_t g[5] = {0b110, 0b100, 0b100, 0b100, 0b110}; return g; }
        case ']': { static const uint8_t g[5] = {0b011, 0b001, 0b001, 0b001, 0b011}; return g; }
        case '(': { static const uint8_t g[5] = {0b010, 0b100, 0b100, 0b100, 0b010}; return g; }
        case ')': { static const uint8_t g[5] = {0b010, 0b001, 0b001, 0b001, 0b010}; return g; }
        case ',': { static const uint8_t g[5] = {0b000, 0b000, 0b000, 0b010, 0b100}; return g; }
        case '.': { static const uint8_t g[5] = {0b000, 0b000, 0b000, 0b000, 0b010}; return g; }
        case '|': { static const uint8_t g[5] = {0b010, 0b010, 0b010, 0b010, 0b010}; return g; }
        case '#': { static const uint8_t g[5] = {0b000, 0b101, 0b111, 0b101, 0b000}; return g; }
        case '!': { static const uint8_t g[5] = {0b010, 0b010, 0b010, 0b000, 0b010}; return g; }
        case '?': { static const uint8_t g[5] = {0b110, 0b001, 0b010, 0b000, 0b010}; return g; }
        case '_': { static const uint8_t g[5] = {0b000, 0b000, 0b000, 0b000, 0b111}; return g; }
        case '>': { static const uint8_t g[5] = {0b100, 0b010, 0b001, 0b010, 0b100}; return g; }
        case '<': { static const uint8_t g[5] = {0b001, 0b010, 0b100, 0b010, 0b001}; return g; }
        default: return GLYPH_UNKNOWN;
    }
}

void draw_text3x5(SDL_Renderer* renderer, const std::string& text, int x, int y, int px_size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255){
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int cur_x = x;
    for(char c : text){
        const uint8_t* glyph = get_font3x5_glyph(c);
        for(int row = 0; row < 5; row++){
            uint8_t byte = glyph[row];
            for(int col = 0; col < 3; col++){
                if((byte & (0x4 >> col)) != 0){
                    SDL_Rect rect = { cur_x + col * px_size, y + row * px_size, px_size, px_size };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        cur_x += 4 * px_size; // 3 pixels glyph + 1 pixel gap
    }
}

std::string get_save_filename(int slot){
    return "savestate_slot" + std::to_string(slot) + ".c8s";
}

std::string disassemble_opcode(uint16_t op, uint16_t addr){
    std::ostringstream oss;
    oss << "[0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(3) << addr << "] 0x"
        << std::setw(4) << op << "  ";

    uint8_t x = (op & 0x0F00) >> 8;
    uint8_t y = (op & 0x00F0) >> 4;
    uint8_t n = op & 0x000F;
    uint8_t kk = op & 0x00FF;
    uint16_t nnn = op & 0x0FFF;

    switch(op & 0xF000){
        case 0x0000:
            if(op == 0x00E0) oss << "CLS";
            else if(op == 0x00EE) oss << "RET";
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
            switch(n){
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
            if(kk == 0x9E) oss << "SKP V" << (int)x;
            else if(kk == 0xA1) oss << "SKNP V" << (int)x;
            else oss << "UNKNOWN";
            break;
        case 0xF000:
            switch(kk){
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

int disassemble_file(const std::string& filename){
    std::ifstream file(filename, std::ios::binary);
    if(!file.is_open()){
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return 1;
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::cout << "--- Disassembly: " << filename << " (" << buffer.size() << " bytes) ---" << std::endl;
    for(size_t i = 0; i + 1 < buffer.size(); i += 2){
        uint16_t addr = 0x200 + (uint16_t)i;
        uint16_t opcode = (buffer[i] << 8) | buffer[i+1];
        std::cout << disassemble_opcode(opcode, addr) << std::endl;
    }
    return 0;
}

void audio_callback(void* userdata, uint8_t* stream, int len){
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len/2;

    bool* beeping = (bool*) userdata;
    for(int i=0; i<samples; i++){
        if(*beeping){
            // Generating 440Hz square wave at 44100Hz (period ~100 samples, flip every 50)
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        }
        else{
            audio_buffer[i] = 0; // Silence
            sample_index = 0;
        }
    }
}

void draw_hud(SDL_Renderer* renderer, const Chip8& chip8, const App& app, int window_width, int window_height){
    int px = std::max(2, app.scale / 6);
    int line_h = 7 * px;

    // Draw HUD if enabled
    if(app.show_hud){
        std::vector<std::string> lines;
        lines.push_back("ROM: " + app.rom_name + " | " + (app.paused ? "PAUSED" : "RUNNING") + " | ESC: HOME");
        lines.push_back("CYC/FRAME: " + std::to_string(app.cycles_per_frame) + " | PAL: " + PALETTES[app.palette].name);
        lines.push_back("SLOT: " + std::to_string(app.slot) + " | " + (app.status.empty() ? "READY" : app.status));
        lines.push_back("LAST: " + disassemble_opcode(chip8.get_last_opcode(), chip8.get_last_pc()));

        size_t max_len = 0;
        for(const auto& l : lines) max_len = std::max(max_len, l.size());

        int box_w = (int)max_len * 4 * px + 16;
        int box_h = (int)lines.size() * line_h + 12;
        int box_x = 10;
        int box_y = 10;

        // Dark translucent background box
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 215);
        SDL_Rect bg = { box_x, box_y, box_w, box_h };
        SDL_RenderFillRect(renderer, &bg);
        // Border
        SDL_SetRenderDrawColor(renderer, 0, 255, 200, 220);
        SDL_RenderDrawRect(renderer, &bg);

        // Draw lines of text
        for(size_t i = 0; i < lines.size(); i++){
            uint8_t r = 255, g = 255, b = 255;
            if(i == 0) { r = 0; g = 255; b = 200; }
            else if(i == 3) { r = 255; g = 220; b = 50; }
            draw_text3x5(renderer, lines[i], box_x + 8, box_y + 6 + (int)i * line_h, px, r, g, b);
        }
    }

    // 5-second controls cheat-sheet when in demo mode
    if(app.demo_mode && (SDL_GetTicks() - app.start_time < 5000)){
        std::vector<std::string> cheat;
        std::string lower = app.rom_name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        cheat.push_back("=== CHIP-8 CONTROLS CHEAT-SHEET ===");
        if(lower.find("pong") != std::string::npos){
            cheat.push_back("P1: [W] UP, [S] DOWN   |   P2: [UP ARROW], [DOWN ARROW]");
        } else if(lower.find("tetris") != std::string::npos){
            cheat.push_back("[A/D] or [ARROWS]: MOVE  |  [W/UP]: ROTATE  |  [S/DOWN]: DROP");
        } else if(lower.find("blinky") != std::string::npos){
            cheat.push_back("[WASD] or [ARROWS]: MOVE PAC-MAN IN 4 DIRECTIONS");
        } else {
            cheat.push_back("CONTROLS: [WASD] or [ARROWS]  |  HEX: 1234/QWER/ASDF/ZXCV");
        }
        cheat.push_back("ESC: HOME MENU   |   +/- : SPEED   |   C/TAB : PALETTE   |   SPACE/P : PAUSE");
        cheat.push_back("F1-F4 : SLOTS   |   F5 : SAVE   |   F9 : LOAD   |   H : TOGGLE HUD");
        int c_px = std::max(2, app.scale / 7);
        int c_line_h = 7 * c_px;
        size_t c_max_len = 0;
        for(const auto& l : cheat) c_max_len = std::max(c_max_len, l.size());

        int c_box_w = (int)c_max_len * 4 * c_px + 16;
        int c_box_h = (int)cheat.size() * c_line_h + 12;
        int c_box_x = (window_width - c_box_w) / 2;
        int c_box_y = window_height - c_box_h - 15;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 15, 30, 235);
        SDL_Rect c_bg = { c_box_x, c_box_y, c_box_w, c_box_h };
        SDL_RenderFillRect(renderer, &c_bg);
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 240);
        SDL_RenderDrawRect(renderer, &c_bg);

        for(size_t i = 0; i < cheat.size(); i++){
            uint8_t r = 255, g = 255, b = 255;
            if(i == 0) { r = 255; g = 215; b = 0; }
            draw_text3x5(renderer, cheat[i], c_box_x + 8, c_box_y + 6 + (int)i * c_line_h, c_px, r, g, b);
        }
    }
}

void draw_graphics(SDL_Renderer* renderer, Chip8& chip8, const App& app, const Palette& pal){
    // Clear screen with palette background
    SDL_SetRenderDrawColor(renderer, pal.bg[0], pal.bg[1], pal.bg[2], 255);
    SDL_RenderClear(renderer);
    // Drawing lit pixels with palette foreground
    SDL_SetRenderDrawColor(renderer, pal.fg[0], pal.fg[1], pal.fg[2], 255);
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(chip8.display[x + (y*64)] == 1){
                SDL_Rect rect = {x * app.scale, y * app.scale, app.scale, app.scale};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    // Draw HUD & overlays
    draw_hud(renderer, chip8, app, 64 * app.scale, 32 * app.scale);

    SDL_RenderPresent(renderer);
}

void update_title(SDL_Window* window, const App& app){
    std::string t = "Chip-8 | " + std::to_string(app.cycles_per_frame) + " cyc/frame | " +
                    PALETTES[app.palette].name + " | Slot " + std::to_string(app.slot);
    if(app.paused) t += " | PAUSED (N=step)";
    if(app.show_hud) t += " | HUD";
    if(!app.status.empty()) t += " | " + app.status;
    SDL_SetWindowTitle(window, t.c_str());
}

void change_speed(App& app, int direction){
    int step = (app.cycles_per_frame < 20) ? 1 : 10;
    app.cycles_per_frame += direction * step;
    if(app.cycles_per_frame < MIN_CYCLES) app.cycles_per_frame = MIN_CYCLES;
    if(app.cycles_per_frame > MAX_CYCLES) app.cycles_per_frame = MAX_CYCLES;
}

void apply_game_key(Chip8& chip8, const App& app, SDL_Keycode k, uint8_t state){
    // 1. Standard Chip-8 16-key hex keypad
    for(int i=0; i<16; i++){
        if(k == keymap[i]) chip8.key[i] = state;
    }

    std::string lower = app.rom_name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // 2. Pong custom controls: Player 1 = W/S, Player 2 = Up/Down arrows
    if(lower.find("pong") != std::string::npos){
        if(k == SDLK_w) chip8.key[1] = state;           // P1 Up
        if(k == SDLK_s) chip8.key[4] = state;           // P1 Down
        if(k == SDLK_UP) chip8.key[12] = state;         // P2 Up (0xC)
        if(k == SDLK_DOWN) chip8.key[13] = state;       // P2 Down (0xD)
    }
    // 3. Tetris custom controls: A/D/Left/Right = Move, W/Up = Rotate, S/Down = Drop
    else if(lower.find("tetris") != std::string::npos){
        if(k == SDLK_a || k == SDLK_LEFT) chip8.key[4] = state;   // Left
        if(k == SDLK_d || k == SDLK_RIGHT) chip8.key[6] = state;  // Right
        if(k == SDLK_w || k == SDLK_UP) chip8.key[5] = state;     // Rotate
        if(k == SDLK_s || k == SDLK_DOWN) chip8.key[7] = state;   // Drop
    }
    // 4. Blinky custom controls: W/Up = Up, S/Down = Down, A/Left = Left, D/Right = Right
    else if(lower.find("blinky") != std::string::npos){
        if(k == SDLK_w || k == SDLK_UP) chip8.key[3] = state;     // Up
        if(k == SDLK_s || k == SDLK_DOWN) chip8.key[6] = state;   // Down
        if(k == SDLK_a || k == SDLK_LEFT) chip8.key[7] = state;   // Left
        if(k == SDLK_d || k == SDLK_RIGHT) chip8.key[8] = state;  // Right
    }
    // 5. Default WASD / Arrow mapping for other games
    else {
        if(k == SDLK_w || k == SDLK_UP) chip8.key[5] = state;     // Up/Action
        if(k == SDLK_s || k == SDLK_DOWN) chip8.key[8] = state;   // Down
        if(k == SDLK_a || k == SDLK_LEFT) chip8.key[7] = state;   // Left
        if(k == SDLK_d || k == SDLK_RIGHT) chip8.key[9] = state;  // Right
    }
}

void handle_input(Chip8& chip8, App& app, bool& running){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            SDL_Keycode k = event.key.keysym.sym;
            // Pressing ESC or M returns to the Home Menu!
            if(k == SDLK_ESCAPE || k == SDLK_m){
                app.in_menu = true;
                app.paused = false;
                for(int i=0; i<16; i++) chip8.key[i] = 0;
                app.status = "Home Menu";
            }
            // Speed controls (+ / -) and reset (Backspace)
            else if(k == SDLK_EQUALS || k == SDLK_PLUS) { change_speed(app, +1); app.status.clear(); }
            else if(k == SDLK_MINUS  || k == SDLK_UNDERSCORE) { change_speed(app, -1); app.status.clear(); }
            else if(k == SDLK_BACKSPACE) { app.cycles_per_frame = DEFAULT_CYCLES; app.status.clear(); }
            // Visual & HUD
            else if(k == SDLK_c || k == SDLK_TAB) { app.palette = (app.palette + 1) % NUM_PALETTES; chip8.draw_flag = true; app.status.clear(); }
            else if(k == SDLK_h) { app.show_hud = !app.show_hud; }
            else if(k == SDLK_SPACE || k == SDLK_p) { app.paused = !app.paused; app.status.clear(); }
            else if(k == SDLK_n && app.paused){
                std::cout << "[STEP] " << disassemble_opcode(chip8.get_current_opcode(), chip8.get_pc()) << std::endl;
                chip8.emulate_cycle();
                app.status = "Step: " + disassemble_opcode(chip8.get_current_opcode(), chip8.get_pc());
            }
            // Savestate slots & ops
            else if(k == SDLK_F1) { app.slot = 1; app.status = "Slot 1 active"; }
            else if(k == SDLK_F2) { app.slot = 2; app.status = "Slot 2 active"; }
            else if(k == SDLK_F3) { app.slot = 3; app.status = "Slot 3 active"; }
            else if(k == SDLK_F4) { app.slot = 4; app.status = "Slot 4 active"; }
            else if(k == SDLK_F5){
                std::string filename = get_save_filename(app.slot);
                if(chip8.save_state(filename)){
                    app.status = "State saved (slot " + std::to_string(app.slot) + ")";
                    std::cout << "Savestate saved to " << filename << std::endl;
                } else {
                    app.status = "Save FAILED (slot " + std::to_string(app.slot) + ")";
                    std::cerr << "Failed to save state to " << filename << std::endl;
                }
            }
            else if(k == SDLK_F9){
                std::string filename = get_save_filename(app.slot);
                if(chip8.load_state(filename)){
                    app.status = "State loaded (slot " + std::to_string(app.slot) + ")";
                    std::cout << "Savestate loaded from " << filename << std::endl;
                } else {
                    app.status = "Load FAILED (slot " + std::to_string(app.slot) + ")";
                    std::cerr << "Failed to load state from " << filename << " (corrupt/missing/wrong version)" << std::endl;
                }
            }

            // Apply game inputs (WASD, Arrows, and Hex keypad)
            apply_game_key(chip8, app, k, 1);
        }
        if(event.type == SDL_KEYUP){
            apply_game_key(chip8, app, event.key.keysym.sym, 0);
        }
    }
}

void dump_screen(const Chip8& chip8){
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            std::cout << (chip8.display[x + y*64] ? '#' : '.');
        }
        std::cout << "\n";
    }
    std::cout << std::flush;
}

struct RomEntry {
    std::string title;
    std::string subtitle;
    std::string path;
};

std::vector<RomEntry> get_available_roms(){
    std::vector<RomEntry> list;
    if(std::filesystem::exists("roms/Pong.ch8")){
        list.push_back({"1. PONG", "2-PLAYER TENNIS (P1: W/S | P2: ARROWS)", "roms/Pong.ch8"});
    }
    if(std::filesystem::exists("roms/Tetris.ch8")){
        list.push_back({"2. TETRIS", "PUZZLE (A/D/ARROWS: MOVE, W/UP: ROTATE)", "roms/Tetris.ch8"});
    }
    if(std::filesystem::exists("roms/Blinky.ch8")){
        list.push_back({"3. BLINKY", "PAC-MAN ARCADE (WASD / ARROWS: MOVE)", "roms/Blinky.ch8"});
    }
    if(std::filesystem::exists("roms")){
        for(const auto& entry : std::filesystem::recursive_directory_iterator("roms")){
            if(entry.is_regular_file() && entry.path().extension() == ".ch8"){
                std::string p = entry.path().string();
                std::replace(p.begin(), p.end(), '\\', '/');
                if(p == "roms/Pong.ch8" || p == "roms/Tetris.ch8" || p == "roms/Blinky.ch8"){
                    continue;
                }
                std::string stem = entry.path().stem().string();
                std::string title = std::to_string(list.size() + 1) + ". " + stem;
                std::string sub = (p.find("tests") != std::string::npos) ? "TEST SUITE ROM" : "CHIP-8 ROM";
                list.push_back({title, sub, p});
            }
        }
    }
    return list;
}

void draw_menu(SDL_Renderer* renderer, const App& app, const std::vector<RomEntry>& roms, int selected_index, const Palette& pal){
    int win_w = 64 * app.scale;
    int win_h = 32 * app.scale;

    // Clear background with palette background
    SDL_SetRenderDrawColor(renderer, pal.bg[0], pal.bg[1], pal.bg[2], 255);
    SDL_RenderClear(renderer);

    int px = std::max(2, app.scale / 6);
    int line_h = 7 * px;

    // Top Header Banner
    int header_box_w = win_w - 40;
    int header_box_h = 2 * line_h + 10;
    int header_x = 20;
    int header_y = 10;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect h_rect = { header_x, header_y, header_box_w, header_box_h };
    SDL_RenderFillRect(renderer, &h_rect);
    SDL_SetRenderDrawColor(renderer, pal.fg[0], pal.fg[1], pal.fg[2], 200);
    SDL_RenderDrawRect(renderer, &h_rect);

    std::string title = "=== CHIP-8 RETRO CONSOLE ===";
    std::string sub = "SELECT A GAME TO PLAY";
    draw_text3x5(renderer, title, header_x + (header_box_w - (int)title.size() * 4 * px) / 2, header_y + 4, px, 255, 215, 0);
    draw_text3x5(renderer, sub, header_x + (header_box_w - (int)sub.size() * 4 * px) / 2, header_y + 4 + line_h, px, 220, 225, 235);

    // List of ROMs
    int list_y = header_y + header_box_h + 8;
    int footer_h = 2 * line_h + 10;
    int available_h = win_h - list_y - footer_h - 12;
    int count = (int)roms.size();
    int row_h = count > 0 ? (available_h / count) : 24;
    if(row_h > line_h + 10) row_h = line_h + 10;
    if(row_h < line_h + 2) row_h = line_h + 2;

    for(int i = 0; i < count; i++){
        int iy = list_y + i * row_h;
        if(iy + row_h > win_h - footer_h - 6) break;

        bool is_sel = (i == selected_index);
        SDL_Rect item_rect = { 20, iy, win_w - 40, row_h - 2 };

        if(is_sel){
            // Highlight background
            SDL_SetRenderDrawColor(renderer, 45, 55, 75, 220);
            SDL_RenderFillRect(renderer, &item_rect);
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            SDL_RenderDrawRect(renderer, &item_rect);

            std::string cursor = "> ";
            draw_text3x5(renderer, cursor + roms[i].title, 26, iy + (row_h - 5 * px) / 2, px, 255, 255, 255);

            int sub_x = win_w - 26 - (int)roms[i].subtitle.size() * 4 * px;
            if(sub_x > 26 + (int)(cursor + roms[i].title).size() * 4 * px + 8){
                draw_text3x5(renderer, roms[i].subtitle, sub_x, iy + (row_h - 5 * px) / 2, px, 255, 215, 0);
            }
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90);
            SDL_RenderFillRect(renderer, &item_rect);

            std::string cursor = "  ";
            draw_text3x5(renderer, cursor + roms[i].title, 26, iy + (row_h - 5 * px) / 2, px, 180, 190, 205);

            int sub_x = win_w - 26 - (int)roms[i].subtitle.size() * 4 * px;
            if(sub_x > 26 + (int)(cursor + roms[i].title).size() * 4 * px + 8){
                draw_text3x5(renderer, roms[i].subtitle, sub_x, iy + (row_h - 5 * px) / 2, px, 130, 140, 150);
            }
        }
    }

    // Footer Help Bar
    int footer_y = win_h - footer_h - 6;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect f_rect = { 20, footer_y, win_w - 40, footer_h };
    SDL_RenderFillRect(renderer, &f_rect);
    SDL_SetRenderDrawColor(renderer, 80, 85, 95, 255);
    SDL_RenderDrawRect(renderer, &f_rect);

    std::string f1 = "UP/DOWN: SELECT  |  ENTER/SPACE/CLICK: PLAY  |  1-" + std::to_string(std::min(count, 9)) + ": QUICK LAUNCH";
    std::string f2 = "C/TAB: PALETTE (" + std::string(pal.name) + ")  |  ESC: QUIT";
    draw_text3x5(renderer, f1, 26, footer_y + 4, px, 0, 255, 200);
    draw_text3x5(renderer, f2, 26, footer_y + 4 + line_h, px, 200, 205, 215);

    SDL_RenderPresent(renderer);
}

void handle_menu_input(Chip8& chip8, App& app, const std::vector<RomEntry>& roms, int& selected_index, bool& running){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        else if(event.type == SDL_KEYDOWN){
            SDL_Keycode k = event.key.keysym.sym;
            if(k == SDLK_ESCAPE){
                running = false;
            } else if(k == SDLK_UP || k == SDLK_w){
                if(!roms.empty()){
                    selected_index = (selected_index - 1 + (int)roms.size()) % (int)roms.size();
                }
            } else if(k == SDLK_DOWN || k == SDLK_s){
                if(!roms.empty()){
                    selected_index = (selected_index + 1) % (int)roms.size();
                }
            } else if(k >= SDLK_1 && k <= SDLK_9){
                int idx = k - SDLK_1;
                if(idx < (int)roms.size()){
                    selected_index = idx;
                    chip8 = Chip8();
                    if(chip8.load_rom(roms[selected_index].path)){
                        app.rom_name = std::filesystem::path(roms[selected_index].path).filename().string();
                        app.in_menu = false;
                        app.start_time = SDL_GetTicks();
                        app.status = "Playing " + app.rom_name;
                    }
                }
            } else if(k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE){
                if(!roms.empty() && selected_index >= 0 && selected_index < (int)roms.size()){
                    chip8 = Chip8();
                    if(chip8.load_rom(roms[selected_index].path)){
                        app.rom_name = std::filesystem::path(roms[selected_index].path).filename().string();
                        app.in_menu = false;
                        app.start_time = SDL_GetTicks();
                        app.status = "Playing " + app.rom_name;
                    }
                }
            } else if(k == SDLK_c || k == SDLK_TAB){
                app.palette = (app.palette + 1) % NUM_PALETTES;
            }
        } else if(event.type == SDL_MOUSEBUTTONDOWN){
            if(event.button.button == SDL_BUTTON_LEFT){
                int my = event.button.y;
                int mx = event.button.x;
                int win_w = 64 * app.scale;
                int win_h = 32 * app.scale;
                int px = std::max(2, app.scale / 6);
                int line_h = 7 * px;
                int header_box_h = 2 * line_h + 10;
                int list_y = 10 + header_box_h + 8;
                int footer_h = 2 * line_h + 10;
                int available_h = win_h - list_y - footer_h - 12;
                int count = (int)roms.size();
                int row_h = count > 0 ? (available_h / count) : 24;
                if(row_h > line_h + 10) row_h = line_h + 10;
                if(row_h < line_h + 2) row_h = line_h + 2;

                if(mx >= 20 && mx <= win_w - 20 && my >= list_y){
                    int clicked_idx = (my - list_y) / row_h;
                    if(clicked_idx >= 0 && clicked_idx < count){
                        selected_index = clicked_idx;
                        chip8 = Chip8();
                        if(chip8.load_rom(roms[selected_index].path)){
                            app.rom_name = std::filesystem::path(roms[selected_index].path).filename().string();
                            app.in_menu = false;
                            app.start_time = SDL_GetTicks();
                            app.status = "Playing " + app.rom_name;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv){
    bool headless = false;
    bool dump = false;
    bool disasm_mode = false;
    bool demo_mode = false;
    int scale = -1;
    int cycles = 1000;
    std::string rom_path = "";

    for(int i=1; i<argc; i++){
        std::string arg = argv[i];
        if(arg == "--headless"){
            headless = true;
        } else if(arg == "--dump-screen"){
            dump = true;
        } else if(arg == "--disasm"){
            disasm_mode = true;
        } else if(arg == "--demo"){
            demo_mode = true;
        } else if(arg == "--scale" && i+1 < argc){
            scale = std::stoi(argv[++i]);
        } else if(arg == "--cycles" && i+1 < argc){
            cycles = std::stoi(argv[++i]);
        } else if(arg.rfind("--", 0) != 0){
            rom_path = arg;
        }
    }

    if(disasm_mode){
        if(rom_path.empty()){
            std::cerr << "Usage: " << argv[0] << " --disasm <ROM file>" << std::endl;
            return 1;
        }
        return disassemble_file(rom_path);
    }

    if(headless){
        if(rom_path.empty()){
            rom_path = "roms/Pong.ch8";
        }
        Chip8 chip8;
        if(!chip8.load_rom(rom_path)){
            return 1;
        }
        for(int i=0; i<cycles; i++){
            chip8.emulate_cycle();
            if(i % 10 == 0){
                chip8.update_timers();
            }
        }
        if(dump){
            dump_screen(chip8);
        }
        return 0;
    }

    if(scale <= 0){
        scale = demo_mode ? 20 : DEFAULT_SCALE;
    }

    std::vector<RomEntry> roms = get_available_roms();

    App app;
    app.scale = scale;
    app.demo_mode = demo_mode;
    app.show_hud = demo_mode;
    app.start_time = SDL_GetTicks();

    if(rom_path.empty()){
        app.in_menu = true;
    } else {
        app.in_menu = false;
        app.rom_name = std::filesystem::path(rom_path).filename().string();
    }

    int width = 64 * app.scale;
    int height = 32 * app.scale;

    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Audio setup
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
    if(audio_device == 0) std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    else SDL_PauseAudioDevice(audio_device, 0);

    SDL_Window* window = SDL_CreateWindow("Chip-8 Retro Console", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if(!window){
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if(!renderer){
        std::cerr << "Renderer error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Chip8 chip8;
    if(!app.in_menu){
        if(!chip8.load_rom(rom_path)){
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }
    
    const Uint32 FRAME_MS = 1000 / 60;
    bool running = true;
    int menu_selection = 0;

    while(running){
        Uint32 frame_start = SDL_GetTicks();

        if(app.in_menu){
            handle_menu_input(chip8, app, roms, menu_selection, running);
            draw_menu(renderer, app, roms, menu_selection, PALETTES[app.palette]);
            beeping = false;
            SDL_SetWindowTitle(window, ("Chip-8 Retro Console | Home Menu | Palette: " + std::string(PALETTES[app.palette].name)).c_str());
        } else {
            handle_input(chip8, app, running);
            if(!app.paused){
                for(int i=0; i<app.cycles_per_frame; i++){
                    chip8.emulate_cycle();
                }
                chip8.update_timers(); // exactly once per frame => 60 Hz
            }

            beeping = (!app.paused && chip8.get_sound_timer() > 0);
            draw_graphics(renderer, chip8, app, PALETTES[app.palette]);
            update_title(window, app);
        }

        // Sleep only for what is left of this frame (delay is per FRAME, not per cycle)
        Uint32 elapsed = SDL_GetTicks() - frame_start;
        if(elapsed < FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
    }
    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}