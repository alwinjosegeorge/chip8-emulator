#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <SDL2/SDL.h>
#include "app_state.h"
#include "chip8.h"
#include "ui_font.h"

struct UiClickResult {
    enum Type {
        NONE,
        LAUNCH_ROM,
        RETURN_TO_LAUNCHER,
        TOGGLE_FAVORITE,
        SET_TAB,
        SET_PALETTE,
        TOGGLE_PAUSE,
        STEP_INSTRUCTION,
        RESTART_ROM,
        SAVE_STATE,
        LOAD_STATE,
        SET_SLOT,
        TOGGLE_DEBUGGER,
        TOGGLE_SETTINGS,
        TOGGLE_KEYPAD,
        TOGGLE_SCANLINES,
        TOGGLE_MUTE,
        SET_SPEED,
        RESCAN_ROMS,
        CLEAR_RECENT
    } type = NONE;

    int rom_index = -1;
    int int_val = 0;
};

class UiRenderer {
public:
    UiRenderer();

    void render(SDL_Renderer* renderer, AppContext& ctx, Chip8& chip8, int win_w, int win_h);
    UiClickResult handle_mouse_click(AppContext& ctx, Chip8& chip8, int mx, int my, int win_w, int win_h);
    void handle_mouse_move(AppContext& ctx, int mx, int my, int win_w, int win_h);
    void handle_mouse_wheel(AppContext& ctx, int wheel_y, int win_h);

private:
    void render_header(SDL_Renderer* renderer, AppContext& ctx, int win_w);
    void render_launcher(SDL_Renderer* renderer, AppContext& ctx, int win_w, int win_h);
    void render_gameplay(SDL_Renderer* renderer, AppContext& ctx, Chip8& chip8, int win_w, int win_h);
    void render_debugger(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int w, int h);
    void render_settings_modal(SDL_Renderer* renderer, AppContext& ctx, int win_w, int win_h);
    void render_keypad_modal(SDL_Renderer* renderer, int win_w, int win_h);
    void render_toast(SDL_Renderer* renderer, const Toast& toast, int win_w);

    void draw_procedural_art(SDL_Renderer* renderer, const RomItem& item, int x, int y, int w, int h);
    void draw_glass_panel(SDL_Renderer* renderer, SDL_Rect rect, uint8_t bg_r, uint8_t bg_g, uint8_t bg_b, uint8_t bg_a,
                          uint8_t border_r, uint8_t border_g, uint8_t border_b, uint8_t border_a);
    void draw_button(SDL_Renderer* renderer, SDL_Rect rect, const std::string& label, bool hovered,
                     uint8_t accent_r, uint8_t accent_g, uint8_t accent_b, int font_scale = 1);

    // Mouse hit-testing state
    int hovered_tab = -1;
    int hovered_card = -1;
    int hovered_btn = -1;
};

#endif // UI_RENDERER_H
