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

const int SCALE = 10; // Each pixel is 10x10 screen pixels
const int WIDTH = 64*SCALE;
const int HEIGHT = 32*SCALE;

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
    {"Classic Green",       {  0,  20,   0}, { 51, 255,  51}},
    {"Amber CRT",           { 20,  10,   0}, {255, 176,   0}},
    {"Neon High-Contrast",  {  5,   0,  20}, {  0, 255, 255}},
    {"White-on-Black",      {  0,   0,   0}, {255, 255, 255}},
};
const int NUM_PALETTES = sizeof(PALETTES) / sizeof(PALETTES[0]);

struct App {
    int cycles_per_frame = DEFAULT_CYCLES;
    int palette = 0;
    int slot = 1;
    bool paused = false;
    std::string status;
};

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

void draw_graphics(SDL_Renderer* renderer, Chip8& chip8, const Palette& pal){
    // Clear screen with palette background
    SDL_SetRenderDrawColor(renderer, pal.bg[0], pal.bg[1], pal.bg[2], 255);
    SDL_RenderClear(renderer);
    // Drawing lit pixels with palette foreground
    SDL_SetRenderDrawColor(renderer, pal.fg[0], pal.fg[1], pal.fg[2], 255);
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(chip8.display[x + (y*64)] == 1){
                SDL_Rect rect = {x*SCALE, y*SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void update_title(SDL_Window* window, const App& app){
    std::string t = "Chip-8 | " + std::to_string(app.cycles_per_frame) + " cyc/frame | " +
                    PALETTES[app.palette].name + " | Slot " + std::to_string(app.slot);
    if(app.paused) t += " | PAUSED (N=step)";
    if(!app.status.empty()) t += " | " + app.status;
    SDL_SetWindowTitle(window, t.c_str());
}

void change_speed(App& app, int direction){
    int step = (app.cycles_per_frame < 20) ? 1 : 10;
    app.cycles_per_frame += direction * step;
    if(app.cycles_per_frame < MIN_CYCLES) app.cycles_per_frame = MIN_CYCLES;
    if(app.cycles_per_frame > MAX_CYCLES) app.cycles_per_frame = MAX_CYCLES;
}

void handle_input(Chip8& chip8, App& app, bool& running){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            SDL_Keycode k = event.key.keysym.sym;
            if(k == SDLK_ESCAPE) running = false;
            // Emulator controls
            else if(k == SDLK_UP   || k == SDLK_EQUALS || k == SDLK_PLUS) { change_speed(app, +1); app.status.clear(); }
            else if(k == SDLK_DOWN || k == SDLK_MINUS)                    { change_speed(app, -1); app.status.clear(); }
            else if(k == SDLK_BACKSPACE)                                  { app.cycles_per_frame = DEFAULT_CYCLES; app.status.clear(); }
            else if(k == SDLK_c || k == SDLK_TAB)                         { app.palette = (app.palette + 1) % NUM_PALETTES; chip8.draw_flag = true; app.status.clear(); }
            else if(k == SDLK_SPACE || k == SDLK_p)                       { app.paused = !app.paused; app.status.clear(); }
            else if(k == SDLK_n && app.paused){
                std::cout << "[STEP] " << disassemble_opcode(chip8.get_current_opcode(), chip8.get_pc()) << std::endl;
                chip8.emulate_cycle();
                app.status = "Step: " + disassemble_opcode(chip8.get_current_opcode(), chip8.get_pc());
            }
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

            // Check which Chip-8 key was pressed
            for(int i=0; i<16; i++){
                if(k == keymap[i]) chip8.key[i] = 1;
            }
        }
        if(event.type == SDL_KEYUP){
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 0;
            }
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

std::string browse_roms(const std::string& dir_path){
    std::vector<std::string> rom_files;
    std::string search_dir = dir_path.empty() ? "roms" : dir_path;

    if(std::filesystem::exists(search_dir) && std::filesystem::is_directory(search_dir)){
        for(const auto& entry : std::filesystem::recursive_directory_iterator(search_dir)){
            if(entry.is_regular_file() && entry.path().extension() == ".ch8"){
                rom_files.push_back(entry.path().string());
            }
        }
    }

    if(rom_files.empty()){
        std::cerr << "No .ch8 ROM files found in directory: " << search_dir << std::endl;
        return "";
    }

    std::cout << "\n=== CHIP-8 ROM Browser ===" << std::endl;
    for(size_t i = 0; i < rom_files.size(); i++){
        std::cout << "  [" << (i + 1) << "] " << rom_files[i] << std::endl;
    }
    std::cout << "Select ROM number (1-" << rom_files.size() << ") [default: 1]: ";
    std::string line;
    if(std::getline(std::cin, line) && !line.empty()){
        try {
            int choice = std::stoi(line);
            if(choice >= 1 && choice <= (int)rom_files.size()){
                return rom_files[choice - 1];
            }
        } catch(...) {}
    }
    return rom_files[0];
}

int main(int argc, char** argv){
    bool headless = false;
    bool dump = false;
    bool disasm_mode = false;
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

    if(rom_path.empty() || std::filesystem::is_directory(rom_path)){
        rom_path = browse_roms(rom_path);
        if(rom_path.empty()){
            return 1;
        }
    }

    if(headless){
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

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
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
    if(!chip8.load_rom(rom_path)){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    App app;
    const Uint32 FRAME_MS = 1000 / 60;
    bool running = true;
    while(running){
        Uint32 frame_start = SDL_GetTicks();

        handle_input(chip8, app, running);
        if(!app.paused){
            for(int i=0; i<app.cycles_per_frame; i++){
                chip8.emulate_cycle();
            }
            chip8.update_timers(); // exactly once per frame => 60 Hz
        }

        beeping = (!app.paused && chip8.get_sound_timer() > 0);
        draw_graphics(renderer, chip8, PALETTES[app.palette]);
        update_title(window, app);

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