#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>
#include <set>
#include <cstdint>
#include <algorithm>
#include <SDL2/SDL.h>

enum class ScreenMode {
    LAUNCHER,
    GAMEPLAY
};

enum class TabFilter {
    ALL,
    FAVORITES,
    RECENT,
    GAMES,
    TESTS
};

enum class ProceduralArtType {
    PONG,
    TETRIS,
    BLINKY,
    LOGO,
    TEST,
    INVADER,
    MAZE,
    GENERIC
};

struct RomItem {
    std::string path;
    std::string filename;
    std::string title;
    std::string category;
    std::string description;
    size_t file_size = 0;
    bool is_favorite = false;
    uint32_t last_played = 0;
    uint32_t seed = 0;
    ProceduralArtType art_type = ProceduralArtType::GENERIC;
};

struct Palette {
    const char* name;
    uint8_t bg[3];
    uint8_t fg[3];
    uint8_t accent[3];
};

extern const Palette PALETTES[6];
extern const int NUM_PALETTES;

struct Toast {
    std::string message;
    uint32_t start_time = 0;
    uint32_t duration_ms = 2200;
    uint8_t r = 0;
    uint8_t g = 255;
    uint8_t b = 200;

    bool is_active() const {
        return !message.empty() && (SDL_GetTicks() - start_time < duration_ms);
    }
};

struct AppConfig {
    int palette = 0;
    int cycles_per_frame = 10;
    bool scanlines_enabled = true;
    bool crt_bezel_enabled = true;
    bool sound_muted = false;
    int savestate_slot = 1;
    std::set<std::string> favorites;
    std::vector<std::string> recent_roms;

    bool save(const std::string& filepath = "chip8_config.ini");
    bool load(const std::string& filepath = "chip8_config.ini");
};

class AppContext {
public:
    AppContext();

    ScreenMode screen = ScreenMode::LAUNCHER;
    TabFilter tab = TabFilter::ALL;
    std::string search_query;
    bool search_focused = false;

    std::vector<RomItem> roms;
    std::vector<int> filtered_indices;
    int selected_rom_index = -1;
    int hovered_rom_index = -1;

    float scroll_y = 0.0f;
    float target_scroll_y = 0.0f;
    float max_scroll_y = 0.0f;

    bool paused = false;
    bool show_debugger = false;
    bool show_settings_modal = false;
    bool show_keypad_modal = false;
    bool demo_mode = false;
    uint32_t game_start_time = 0;

    Toast toast;
    AppConfig config;

    void init();
    void scan_roms(const std::string& root_dir = "roms");
    void update_filter();
    void toggle_favorite(int rom_index);
    void record_played(int rom_index);
    void show_toast(const std::string& message, uint8_t r = 0, uint8_t g = 255, uint8_t b = 200, uint32_t duration = 2200);

    RomItem* get_selected_rom();
    int get_cycles_hz() const { return config.cycles_per_frame * 60; }
};

#endif // APP_STATE_H
