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

void draw_graphics(SDL_Renderer* renderer, Chip8& chip8){
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // Drawing white pixels
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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

void handle_input(Chip8& chip8, bool& running){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE) running = false;
            // Check which Chip-8 key was pressed
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 1;
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

int main(int argc, char** argv){
    bool headless = false;
    bool dump = false;
    int cycles = 1000;
    std::string rom_path = "";

    for(int i=1; i<argc; i++){
        std::string arg = argv[i];
        if(arg == "--headless"){
            headless = true;
        } else if(arg == "--dump-screen"){
            dump = true;
        } else if(arg == "--cycles" && i+1 < argc){
            cycles = std::stoi(argv[++i]);
        } else if(arg.rfind("--", 0) != 0){
            rom_path = arg;
        }
    }

    if(rom_path.empty()){
        std::cerr << "Usage: " << argv[0] << " <ROM file> [--headless] [--cycles N] [--dump-screen]" << std::endl;
        return 1;
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
    
    const Uint32 FRAME_MS = 1000 / 60;
    bool running = true;
    while(running){
        Uint32 frame_start = SDL_GetTicks();

        handle_input(chip8, running);
        for(int i=0; i<10; i++){
            chip8.emulate_cycle();
        }
        chip8.update_timers(); // exactly once per frame => 60 Hz

        beeping = (chip8.get_sound_timer() > 0);
        draw_graphics(renderer, chip8);

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