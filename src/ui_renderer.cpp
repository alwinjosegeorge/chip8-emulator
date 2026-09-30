#include "ui_renderer.h"
#include <iomanip>
#include <sstream>
#include <cmath>

static std::string disassemble_op(uint16_t op, uint16_t addr) {
    std::ostringstream oss;
    oss << "[0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(3) << addr << "] 0x"
        << std::setw(4) << op << " ";

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

static bool point_in_rect(int px, int py, const SDL_Rect& r) {
    return (px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h);
}

UiRenderer::UiRenderer() {}

void UiRenderer::draw_glass_panel(SDL_Renderer* renderer, SDL_Rect rect,
                                  uint8_t bg_r, uint8_t bg_g, uint8_t bg_b, uint8_t bg_a,
                                  uint8_t border_r, uint8_t border_g, uint8_t border_b, uint8_t border_a) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, bg_a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, border_r, border_g, border_b, border_a);
    SDL_RenderDrawRect(renderer, &rect);
}

void UiRenderer::draw_button(SDL_Renderer* renderer, SDL_Rect rect, const std::string& label, bool hovered,
                             uint8_t accent_r, uint8_t accent_g, uint8_t accent_b, int font_scale) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (hovered) {
        SDL_SetRenderDrawColor(renderer, accent_r, accent_g, accent_b, 45);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, accent_r, accent_g, accent_b, 255);
        SDL_RenderDrawRect(renderer, &rect);
        draw_text_centered(renderer, label, rect.x + rect.w / 2, rect.y + (rect.h - 8 * font_scale) / 2, font_scale, accent_r, accent_g, accent_b, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 22, 27, 34, 220);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 60, 68, 77, 255);
        SDL_RenderDrawRect(renderer, &rect);
        draw_text_centered(renderer, label, rect.x + rect.w / 2, rect.y + (rect.h - 8 * font_scale) / 2, font_scale, 200, 210, 220, 255);
    }
}

void UiRenderer::draw_procedural_art(SDL_Renderer* renderer, const RomItem& item, int x, int y, int w, int h) {
    SDL_Rect art_box = { x, y, w, h };
    SDL_SetRenderDrawColor(renderer, 15, 18, 26, 255);
    SDL_RenderFillRect(renderer, &art_box);

    // Subtle dark grid pattern
    SDL_SetRenderDrawColor(renderer, 24, 28, 40, 255);
    for (int gx = x; gx < x + w; gx += 12) SDL_RenderDrawLine(renderer, gx, y, gx, y + h);
    for (int gy = y; gy < y + h; gy += 12) SDL_RenderDrawLine(renderer, x, gy, x + w, gy);

    switch (item.art_type) {
        case ProceduralArtType::PONG: {
            // Court center line
            SDL_SetRenderDrawColor(renderer, 0, 255, 180, 70);
            for (int cy = y + 4; cy < y + h - 4; cy += 8) {
                SDL_Rect dash = { x + w / 2 - 1, cy, 2, 4 };
                SDL_RenderFillRect(renderer, &dash);
            }
            // Left paddle (Player 1)
            SDL_SetRenderDrawColor(renderer, 0, 255, 200, 255);
            SDL_Rect p1 = { x + 16, y + h / 2 - 14, 5, 28 };
            SDL_RenderFillRect(renderer, &p1);
            // Right paddle (Player 2)
            SDL_SetRenderDrawColor(renderer, 0, 220, 255, 255);
            SDL_Rect p2 = { x + w - 21, y + h / 2 - 8, 5, 28 };
            SDL_RenderFillRect(renderer, &p2);
            // Ball with speed motion
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
            SDL_Rect ball = { x + w / 2 + 10, y + h / 2 - 6, 6, 6 };
            SDL_RenderFillRect(renderer, &ball);
            break;
        }
        case ProceduralArtType::TETRIS: {
            // Stack of colored tetrominoes
            int bs = 10;
            // Cyan I-piece
            SDL_SetRenderDrawColor(renderer, 0, 240, 240, 240);
            for (int i = 0; i < 4; i++) {
                SDL_Rect b = { x + w / 2 - 20 + i * bs, y + h - 18, bs - 1, bs - 1 };
                SDL_RenderFillRect(renderer, &b);
            }
            // Purple T-piece
            SDL_SetRenderDrawColor(renderer, 180, 70, 255, 240);
            SDL_Rect t1 = { x + w / 2 - 10, y + h - 28, bs - 1, bs - 1 };
            SDL_Rect t2 = { x + w / 2, y + h - 28, bs - 1, bs - 1 };
            SDL_Rect t3 = { x + w / 2 + 10, y + h - 28, bs - 1, bs - 1 };
            SDL_Rect t4 = { x + w / 2, y + h - 38, bs - 1, bs - 1 };
            SDL_RenderFillRect(renderer, &t1);
            SDL_RenderFillRect(renderer, &t2);
            SDL_RenderFillRect(renderer, &t3);
            SDL_RenderFillRect(renderer, &t4);
            // Yellow O-piece
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 240);
            SDL_Rect o1 = { x + w / 2 - 32, y + h - 28, bs - 1, bs - 1 };
            SDL_Rect o2 = { x + w / 2 - 22, y + h - 28, bs - 1, bs - 1 };
            SDL_Rect o3 = { x + w / 2 - 32, y + h - 38, bs - 1, bs - 1 };
            SDL_Rect o4 = { x + w / 2 - 22, y + h - 38, bs - 1, bs - 1 };
            SDL_RenderFillRect(renderer, &o1);
            SDL_RenderFillRect(renderer, &o2);
            SDL_RenderFillRect(renderer, &o3);
            SDL_RenderFillRect(renderer, &o4);
            break;
        }
        case ProceduralArtType::BLINKY: {
            // Yellow Pac-Man
            int pcx = x + w / 3;
            int pcy = y + h / 2;
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            SDL_Rect pac = { pcx - 10, pcy - 10, 20, 20 };
            SDL_RenderFillRect(renderer, &pac);
            // Red Ghost
            int gcx = x + 2 * w / 3;
            int gcy = y + h / 2;
            SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
            SDL_Rect ghost = { gcx - 10, gcy - 10, 20, 20 };
            SDL_RenderFillRect(renderer, &ghost);
            // Ghost eyes
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect eye1 = { gcx - 6, gcy - 6, 4, 6 };
            SDL_Rect eye2 = { gcx + 2, gcy - 6, 4, 6 };
            SDL_RenderFillRect(renderer, &eye1);
            SDL_RenderFillRect(renderer, &eye2);
            // Dots
            SDL_SetRenderDrawColor(renderer, 255, 255, 200, 220);
            SDL_Rect d1 = { pcx + 18, pcy - 2, 4, 4 };
            SDL_Rect d2 = { pcx + 32, pcy - 2, 4, 4 };
            SDL_RenderFillRect(renderer, &d1);
            SDL_RenderFillRect(renderer, &d2);
            break;
        }
        case ProceduralArtType::LOGO: {
            // Chip circuit box
            int cx = x + w / 2;
            int cy = y + h / 2;
            SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
            SDL_Rect chip = { cx - 24, cy - 16, 48, 32 };
            SDL_RenderDrawRect(renderer, &chip);
            draw_text_centered(renderer, "CHIP-8", cx, cy - 4, 1, 0, 240, 255);
            // Pins
            for (int px_offset = -18; px_offset <= 18; px_offset += 9) {
                SDL_RenderDrawLine(renderer, cx + px_offset, cy - 16, cx + px_offset, cy - 22);
                SDL_RenderDrawLine(renderer, cx + px_offset, cy + 16, cx + px_offset, cy + 22);
            }
            break;
        }
        case ProceduralArtType::TEST: {
            // Diagnostics terminal symbol
            int cx = x + w / 2;
            int cy = y + h / 2;
            SDL_SetRenderDrawColor(renderer, 50, 255, 100, 240);
            SDL_Rect box = { cx - 35, cy - 18, 70, 36 };
            SDL_RenderDrawRect(renderer, &box);
            draw_text_centered(renderer, "[PASSED]", cx, cy - 9, 1, 50, 255, 100);
            draw_text_centered(renderer, "23/23 OK", cx, cy + 3, 1, 200, 255, 200);
            break;
        }
        default: {
            // Retro arcade cabinet pattern
            int cx = x + w / 2;
            int cy = y + h / 2;
            SDL_SetRenderDrawColor(renderer, 255, 120, 0, 220);
            SDL_Rect cab = { cx - 22, cy - 20, 44, 40 };
            SDL_RenderDrawRect(renderer, &cab);
            draw_text_centered(renderer, "ARCADE", cx, cy - 5, 1, 255, 180, 50);
            break;
        }
    }
}

void UiRenderer::render_header(SDL_Renderer* renderer, AppContext& ctx, int win_w) {
    SDL_Rect header_rect = { 0, 0, win_w, 64 };
    draw_glass_panel(renderer, header_rect, 13, 17, 23, 245, 33, 38, 45, 255);

    // 1. Logo "CHIP//8"
    draw_text_shadow(renderer, "CHIP//8", 24, 16, 2, 0, 240, 230, 255);
    draw_text(renderer, "ARCADE", 146, 23, 1, 255, 180, 0, 240);

    // 2. Tabs
    const char* tab_names[] = { "ALL ROMS", "FAVORITES", "RECENT", "GAMES", "TESTS" };
    int tab_x = 240;
    int tab_w = 100;
    for (int i = 0; i < 5; i++) {
        SDL_Rect t_rect = { tab_x + i * tab_w, 14, tab_w - 8, 36 };
        bool is_active = ((int)ctx.tab == i);
        bool is_hover = (hovered_tab == i);

        if (is_active) {
            draw_text_centered(renderer, tab_names[i], t_rect.x + t_rect.w / 2, t_rect.y + 12, 1, 0, 255, 240, 255);
            // Active underline indicator
            SDL_SetRenderDrawColor(renderer, 0, 255, 240, 255);
            SDL_Rect bar = { t_rect.x + 8, t_rect.y + t_rect.h - 4, t_rect.w - 16, 3 };
            SDL_RenderFillRect(renderer, &bar);
        } else {
            uint8_t c = is_hover ? 230 : 140;
            draw_text_centered(renderer, tab_names[i], t_rect.x + t_rect.w / 2, t_rect.y + 12, 1, c, c, c, 255);
        }
    }

    // 3. Search Bar
    int search_w = 220;
    int search_x = win_w - search_w - 240;
    SDL_Rect s_rect = { search_x, 14, search_w, 36 };
    uint8_t s_border_c = ctx.search_focused ? 240 : 60;
    draw_glass_panel(renderer, s_rect, 22, 27, 34, 255, 0, s_border_c, s_border_c, 255);
    draw_icon_search(renderer, s_rect.x + 16, s_rect.y + 18, 14, 140, 150, 160);

    std::string disp_query = ctx.search_query.empty() ? (ctx.search_focused ? "|" : "Search ROMs...") : ctx.search_query;
    uint8_t q_col = ctx.search_query.empty() ? 100 : 230;
    draw_text(renderer, disp_query, s_rect.x + 32, s_rect.y + 14, 1, q_col, q_col, q_col, 255);

    // 4. Rescan Button
    SDL_Rect rescan_rect = { win_w - 225, 14, 100, 36 };
    draw_button(renderer, rescan_rect, "RESCAN", (hovered_btn == 1), 0, 255, 200, 1);
    draw_icon_restart(renderer, rescan_rect.x + 14, rescan_rect.y + 18, 12, 0, 255, 200);

    // 5. Settings Button
    SDL_Rect set_rect = { win_w - 115, 14, 95, 36 };
    draw_button(renderer, set_rect, "SETTINGS", (hovered_btn == 2), 255, 190, 40, 1);
    draw_icon_gear(renderer, set_rect.x + 14, set_rect.y + 18, 12, 255, 190, 40);
}

void UiRenderer::render_launcher(SDL_Renderer* renderer, AppContext& ctx, int win_w, int win_h) {
    // 1. Dark background
    SDL_SetRenderDrawColor(renderer, 10, 13, 18, 255);
    SDL_RenderClear(renderer);

    int start_y = 74 - (int)ctx.scroll_y;

    // 2. Hero Section
    int hero_h = 100;
    SDL_Rect hero_rect = { 30, start_y, win_w - 60, hero_h };
    draw_glass_panel(renderer, hero_rect, 18, 24, 34, 220, 38, 48, 62, 255);

    // Hero title & subtitle
    draw_text_shadow(renderer, "YOUR CHIP-8 ARCADE", hero_rect.x + 24, hero_rect.y + 20, 2, 0, 245, 230, 255);
    draw_text(renderer, "Classic games. Modern emulation. Select a ROM to play instantly.", hero_rect.x + 25, hero_rect.y + 58, 1, 140, 155, 175, 255);

    // Quick launch featured game (if any ROMs exist)
    if (!ctx.roms.empty()) {
        SDL_Rect q_btn = { hero_rect.x + hero_rect.w - 220, hero_rect.y + 28, 190, 44 };
        draw_button(renderer, q_btn, "PLAY PONG", (hovered_btn == 99), 0, 255, 200, 1);
        draw_icon_play(renderer, q_btn.x + 22, q_btn.y + 22, 14, 0, 255, 200);
    }

    int grid_y = start_y + hero_h + 24;

    // 3. Section Header
    std::string cat_label = "AVAILABLE GAMES & ROMS (" + std::to_string(ctx.filtered_indices.size()) + ")";
    draw_text(renderer, cat_label, 32, grid_y, 1, 0, 220, 200, 240);
    grid_y += 20;

    // 4. Grid of ROM Cards
    int card_w = 280;
    int card_h = 215;
    int gap = 20;
    int cols = std::max(1, (win_w - 60 + gap) / (card_w + gap));

    if (ctx.filtered_indices.empty()) {
        // Empty state
        SDL_Rect empty_box = { 30, grid_y + 20, win_w - 60, 160 };
        draw_glass_panel(renderer, empty_box, 18, 22, 30, 200, 38, 44, 54, 255);
        draw_text_centered(renderer, "NO ROMS FOUND MATCHING FILTER", win_w / 2, empty_box.y + 55, 2, 255, 180, 50, 255);
        draw_text_centered(renderer, "Try changing the search query or selecting 'ALL ROMS'", win_w / 2, empty_box.y + 90, 1, 140, 150, 160, 255);
    } else {
        for (size_t i = 0; i < ctx.filtered_indices.size(); i++) {
            int rom_idx = ctx.filtered_indices[i];
            const auto& item = ctx.roms[rom_idx];

            int col = (int)i % cols;
            int row = (int)i / cols;
            int cx = 30 + col * (card_w + gap);
            int cy = grid_y + row * (card_h + gap);

            // Skip rendering off-screen cards
            if (cy + card_h < 0 || cy > win_h) continue;

            bool is_hover = (hovered_card == (int)i);
            SDL_Rect card_rect = { cx, cy, card_w, card_h };

            // Card Panel
            uint8_t border_r = is_hover ? 0 : 42;
            uint8_t border_g = is_hover ? 240 : 48;
            uint8_t border_b = is_hover ? 220 : 58;
            uint8_t bg_a = is_hover ? 250 : 210;
            draw_glass_panel(renderer, card_rect, 18, 23, 30, bg_a, border_r, border_g, border_b, 255);

            // Procedural Artwork on Card Top
            int art_h = 75;
            draw_procedural_art(renderer, item, cx + 1, cy + 1, card_w - 2, art_h);

            // Favorite Button (Top right of card)
            SDL_Rect fav_btn = { cx + card_w - 32, cy + 6, 24, 24 };
            uint8_t fav_r = item.is_favorite ? 255 : 120;
            uint8_t fav_g = item.is_favorite ? 215 : 120;
            uint8_t fav_b = item.is_favorite ? 0 : 120;
            draw_icon_heart(renderer, fav_btn.x + 12, fav_btn.y + 12, 14, item.is_favorite, fav_r, fav_g, fav_b);

            // Title
            draw_text_shadow(renderer, item.title, cx + 14, cy + art_h + 12, 1, 255, 255, 255, 255);

            // Category Capsule
            SDL_Rect cat_badge = { cx + 14, cy + art_h + 30, (int)item.category.length() * 8 + 10, 16 };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 30, 45, 60, 220);
            SDL_RenderFillRect(renderer, &cat_badge);
            draw_text(renderer, item.category, cx + 19, cy + art_h + 34, 1, 0, 200, 240, 240);

            // Description / size
            draw_text(renderer, item.description, cx + 14, cy + art_h + 54, 1, 130, 140, 150, 240);

            // Play Button at bottom of card
            SDL_Rect play_btn = { cx + 14, cy + card_h - 40, card_w - 28, 28 };
            draw_button(renderer, play_btn, "PLAY", is_hover, 0, 255, 200, 1);
            draw_icon_play(renderer, play_btn.x + play_btn.w / 2 - 30, play_btn.y + 14, 12, 0, 255, 200);
        }
    }

    // Calculate maximum scroll
    int total_rows = ((int)ctx.filtered_indices.size() + cols - 1) / cols;
    int content_bottom = grid_y + total_rows * (card_h + gap) + 40;
    ctx.max_scroll_y = std::max(0.0f, (float)(content_bottom - win_h));

    // Render Sticky Header on top of scrolled content
    render_header(renderer, ctx, win_w);
}

void UiRenderer::render_gameplay(SDL_Renderer* renderer, AppContext& ctx, Chip8& chip8, int win_w, int win_h) {
    // 1. Background
    SDL_SetRenderDrawColor(renderer, 8, 10, 14, 255);
    SDL_RenderClear(renderer);

    const Palette& pal = PALETTES[ctx.config.palette];

    // 2. Top Navigation Bar (Height: 50px)
    SDL_Rect top_bar = { 0, 0, win_w, 50 };
    draw_glass_panel(renderer, top_bar, 13, 17, 23, 245, 33, 38, 45, 255);

    // [ ⬅ LIBRARY ] Back Button
    SDL_Rect back_btn = { 16, 9, 120, 32 };
    draw_button(renderer, back_btn, "LIBRARY", (hovered_btn == 10), 0, 240, 220, 1);
    draw_icon_back(renderer, back_btn.x + 16, back_btn.y + 16, 12, 0, 240, 220);

    // Game Title + Category
    RomItem* rom = ctx.get_selected_rom();
    std::string game_title = rom ? rom->title : "CHIP-8 ROM";
    draw_text_shadow(renderer, game_title, 150, 17, 2, 255, 255, 255, 255);

    // State Badge: RUNNING / PAUSED
    SDL_Rect state_pill = { 340, 12, 90, 26 };
    if (ctx.paused) {
        draw_glass_panel(renderer, state_pill, 50, 40, 10, 220, 255, 180, 0, 255);
        draw_text_centered(renderer, "PAUSED", state_pill.x + state_pill.w / 2, state_pill.y + 9, 1, 255, 200, 50);
    } else {
        draw_glass_panel(renderer, state_pill, 10, 45, 25, 220, 0, 255, 120, 255);
        draw_text_centered(renderer, "RUNNING", state_pill.x + state_pill.w / 2, state_pill.y + 9, 1, 50, 255, 150);
    }

    // Audio Mute Toggle Button
    SDL_Rect mute_btn = { win_w - 460, 9, 85, 32 };
    std::string mute_lbl = ctx.config.sound_muted ? "MUTED" : "SOUND";
    draw_button(renderer, mute_btn, mute_lbl, (hovered_btn == 11), 0, 220, 255, 1);
    draw_icon_volume(renderer, mute_btn.x + 14, mute_btn.y + 16, 12, ctx.config.sound_muted, 0, 220, 255);

    // Palette Theme Button
    SDL_Rect pal_btn = { win_w - 365, 9, 140, 32 };
    draw_button(renderer, pal_btn, pal.name, (hovered_btn == 12), pal.accent[0], pal.accent[1], pal.accent[2], 1);

    // Debugger Toggle Button
    SDL_Rect deb_btn = { win_w - 215, 9, 105, 32 };
    uint8_t deb_r = ctx.show_debugger ? 0 : 180;
    uint8_t deb_g = ctx.show_debugger ? 255 : 180;
    uint8_t deb_b = ctx.show_debugger ? 200 : 180;
    draw_button(renderer, deb_btn, "DEBUGGER", (hovered_btn == 13), deb_r, deb_g, deb_b, 1);
    draw_icon_bug(renderer, deb_btn.x + 14, deb_btn.y + 16, 12, deb_r, deb_g, deb_b);

    // Settings Button
    SDL_Rect set_btn = { win_w - 100, 9, 85, 32 };
    draw_button(renderer, set_btn, "CONFIG", (hovered_btn == 14), 255, 180, 0, 1);
    draw_icon_gear(renderer, set_btn.x + 14, set_btn.y + 16, 12, 255, 180, 0);

    // 3. Center CHIP-8 Display Canvas
    int avail_w = ctx.show_debugger ? (win_w - 360) : win_w;
    int avail_h = win_h - 110; // 50px top + 60px bottom

    int scale_x = avail_w / 64;
    int scale_y = avail_h / 32;
    int chip_scale = std::clamp(std::min(scale_x, scale_y), 6, 16);

    int disp_w = 64 * chip_scale;
    int disp_h = 32 * chip_scale;
    int disp_x = (avail_w - disp_w) / 2;
    int disp_y = 50 + (avail_h - disp_h) / 2;

    // Bezel border
    if (ctx.config.crt_bezel_enabled) {
        SDL_Rect bezel = { disp_x - 6, disp_y - 6, disp_w + 12, disp_h + 12 };
        draw_glass_panel(renderer, bezel, 16, 20, 26, 255, pal.accent[0], pal.accent[1], pal.accent[2], 120);
    }

    // Clear display canvas with palette background
    SDL_SetRenderDrawColor(renderer, pal.bg[0], pal.bg[1], pal.bg[2], 255);
    SDL_Rect canvas = { disp_x, disp_y, disp_w, disp_h };
    SDL_RenderFillRect(renderer, &canvas);

    // Draw active pixels
    SDL_SetRenderDrawColor(renderer, pal.fg[0], pal.fg[1], pal.fg[2], 255);
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (chip8.display[cx + cy * 64] != 0) {
                SDL_Rect px = { disp_x + cx * chip_scale, disp_y + cy * chip_scale, chip_scale, chip_scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }

    // Subtle CRT Scanlines
    if (ctx.config.scanlines_enabled) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 55);
        for (int ly = disp_y; ly < disp_y + disp_h; ly += std::max(2, chip_scale / 2)) {
            SDL_RenderDrawLine(renderer, disp_x, ly, disp_x + disp_w, ly);
        }
    }

    // 4. Debugger Panel (if enabled)
    if (ctx.show_debugger) {
        render_debugger(renderer, chip8, win_w - 350, 56, 335, win_h - 120);
    }

    // 5. Bottom Controls Bar (Height: 60px)
    SDL_Rect btm_bar = { 0, win_h - 60, win_w, 60 };
    draw_glass_panel(renderer, btm_bar, 13, 17, 23, 245, 33, 38, 45, 255);

    // Play / Pause Button
    SDL_Rect play_pause_btn = { 20, win_h - 48, 100, 36 };
    std::string pp_lbl = ctx.paused ? "RESUME" : "PAUSE";
    draw_button(renderer, play_pause_btn, pp_lbl, (hovered_btn == 20), 0, 240, 220, 1);
    if (ctx.paused) draw_icon_play(renderer, play_pause_btn.x + 14, play_pause_btn.y + 18, 12, 0, 240, 220);
    else draw_icon_pause(renderer, play_pause_btn.x + 14, play_pause_btn.y + 18, 12, 0, 240, 220);

    // Restart Button
    SDL_Rect rst_btn = { 130, win_h - 48, 95, 36 };
    draw_button(renderer, rst_btn, "RESTART", (hovered_btn == 21), 255, 120, 50, 1);
    draw_icon_restart(renderer, rst_btn.x + 14, rst_btn.y + 18, 12, 255, 120, 50);

    // Speed Slider / Buttons
    int speed_x = 245;
    SDL_Rect spd_dec = { speed_x, win_h - 48, 30, 36 };
    SDL_Rect spd_inc = { speed_x + 175, win_h - 48, 30, 36 };
    draw_button(renderer, spd_dec, "-", (hovered_btn == 22), 0, 220, 255, 1);
    draw_button(renderer, spd_inc, "+", (hovered_btn == 23), 0, 220, 255, 1);

    SDL_Rect spd_box = { speed_x + 35, win_h - 48, 135, 36 };
    draw_glass_panel(renderer, spd_box, 18, 22, 28, 220, 40, 48, 58, 255);
    std::string spd_str = std::to_string(ctx.config.cycles_per_frame) + " cyc/f (" + std::to_string(ctx.get_cycles_hz()) + "Hz)";
    draw_text_centered(renderer, spd_str, spd_box.x + spd_box.w / 2, spd_box.y + 14, 1, 0, 240, 255);

    // Savestate Slot Selector
    int slot_x = speed_x + 225;
    draw_text(renderer, "SLOT:", slot_x, win_h - 35, 1, 140, 150, 160);
    slot_x += 45;
    for (int s = 1; s <= 4; s++) {
        SDL_Rect s_btn = { slot_x + (s - 1) * 32, win_h - 48, 28, 36 };
        bool is_active = (ctx.config.savestate_slot == s);
        draw_button(renderer, s_btn, std::to_string(s), (hovered_btn == 30 + s), is_active ? 255 : 120, is_active ? 200 : 120, is_active ? 0 : 120, 1);
    }

    // Save & Load Buttons
    int save_x = slot_x + 140;
    SDL_Rect save_btn = { save_x, win_h - 48, 100, 36 };
    SDL_Rect load_btn = { save_x + 110, win_h - 48, 100, 36 };
    draw_button(renderer, save_btn, "SAVE (F5)", (hovered_btn == 24), 50, 255, 150, 1);
    draw_icon_floppy(renderer, save_btn.x + 12, save_btn.y + 18, 12, 50, 255, 150);
    draw_button(renderer, load_btn, "LOAD (F9)", (hovered_btn == 25), 0, 220, 255, 1);

    // Step Instruction Button (active when paused)
    if (ctx.paused) {
        SDL_Rect step_btn = { save_x + 220, win_h - 48, 95, 36 };
        draw_button(renderer, step_btn, "STEP (N)", (hovered_btn == 26), 255, 215, 0, 1);
        draw_icon_step(renderer, step_btn.x + 14, step_btn.y + 18, 12, 255, 215, 0);
    }

    // Keypad Help Button
    SDL_Rect key_btn = { win_w - 120, win_h - 48, 100, 36 };
    draw_button(renderer, key_btn, "KEYPAD", (hovered_btn == 27), 200, 160, 255, 1);
}

void UiRenderer::render_debugger(SDL_Renderer* renderer, const Chip8& chip8, int x, int y, int w, int h) {
    SDL_Rect panel = { x, y, w, h };
    draw_glass_panel(renderer, panel, 14, 18, 26, 245, 0, 200, 240, 180);

    // Panel Header
    draw_text_shadow(renderer, "CHIP-8 CORE DEBUGGER", x + 16, y + 14, 1, 0, 240, 255);
    SDL_SetRenderDrawColor(renderer, 0, 200, 240, 100);
    SDL_RenderDrawLine(renderer, x + 16, y + 28, x + w - 16, y + 28);

    int cur_y = y + 36;

    // 1. Registers V0 - VF
    draw_text(renderer, "REGISTERS (V0-VF):", x + 16, cur_y, 1, 255, 200, 50);
    cur_y += 14;

    for (int i = 0; i < 8; i++) {
        std::ostringstream ss1, ss2;
        ss1 << "V" << std::uppercase << std::hex << i << ": 0x" << std::setw(2) << std::setfill('0') << (int)chip8.get_v(i);
        ss2 << "V" << std::uppercase << std::hex << (i + 8) << ": 0x" << std::setw(2) << std::setfill('0') << (int)chip8.get_v(i + 8);

        uint8_t c1 = (i == 0xF && chip8.get_v(i) != 0) ? 255 : 220;
        uint8_t c2 = (i + 8 == 0xF && chip8.get_v(i + 8) != 0) ? 255 : 220;

        draw_text(renderer, ss1.str(), x + 16, cur_y, 1, c1, c1, 255);
        draw_text(renderer, ss2.str(), x + 170, cur_y, 1, c2, c2, 255);
        cur_y += 12;
    }
    cur_y += 8;

    // 2. Program Counter & Index Register
    std::ostringstream oss_pc, oss_i;
    oss_pc << "PC: 0x" << std::uppercase << std::hex << std::setw(3) << std::setfill('0') << chip8.get_pc();
    oss_i  << "I : 0x" << std::uppercase << std::hex << std::setw(3) << std::setfill('0') << chip8.get_index();
    draw_text(renderer, oss_pc.str(), x + 16, cur_y, 1, 0, 255, 180);
    draw_text(renderer, oss_i.str(), x + 170, cur_y, 1, 0, 255, 180);
    cur_y += 16;

    // 3. Timers & Stack Pointer
    std::ostringstream oss_dt, oss_st, oss_sp;
    oss_dt << "DT: " << (int)chip8.get_delay_timer();
    oss_st << "ST: " << (int)chip8.get_sound_timer();
    oss_sp << "SP: " << (int)chip8.get_sp() << "/16";
    draw_text(renderer, oss_dt.str(), x + 16, cur_y, 1, 255, 180, 50);
    draw_text(renderer, oss_st.str(), x + 110, cur_y, 1, 255, 180, 50);
    draw_text(renderer, oss_sp.str(), x + 200, cur_y, 1, 255, 180, 50);
    cur_y += 18;

    // 4. Current Opcode & Disassembly
    draw_text(renderer, "CURRENT INSTRUCTION:", x + 16, cur_y, 1, 255, 200, 50);
    cur_y += 14;
    std::string disasm = disassemble_op(chip8.get_current_opcode(), chip8.get_pc());
    draw_text(renderer, disasm, x + 16, cur_y, 1, 255, 255, 100);
    cur_y += 20;

    // 5. Memory View around PC
    draw_text(renderer, "MEMORY NEAR PC:", x + 16, cur_y, 1, 255, 200, 50);
    cur_y += 14;
    uint16_t cur_pc = chip8.get_pc();
    for (int offset = -2; offset <= 6; offset += 2) {
        uint16_t addr = (cur_pc + offset) & 0xFFF;
        uint16_t op = (chip8.get_memory(addr) << 8) | chip8.get_memory(addr + 1);
        std::string line = disassemble_op(op, addr);
        uint8_t r = (offset == 0) ? 255 : 150;
        uint8_t g = (offset == 0) ? 255 : 160;
        uint8_t b = (offset == 0) ? 100 : 170;
        if (offset == 0) line = "> " + line;
        else line = "  " + line;
        draw_text(renderer, line, x + 16, cur_y, 1, r, g, b);
        cur_y += 12;
    }
}

void UiRenderer::render_settings_modal(SDL_Renderer* renderer, AppContext& ctx, int win_w, int win_h) {
    // Backdrop overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
    SDL_Rect full = { 0, 0, win_w, win_h };
    SDL_RenderFillRect(renderer, &full);

    // Modal dialog
    int modal_w = 700;
    int modal_h = 520;
    SDL_Rect modal = { (win_w - modal_w) / 2, (win_h - modal_h) / 2, modal_w, modal_h };
    draw_glass_panel(renderer, modal, 16, 20, 28, 255, 0, 200, 230, 220);

    // Header
    draw_text_shadow(renderer, "EMULATOR SETTINGS", modal.x + 24, modal.y + 20, 2, 0, 240, 255);
    SDL_SetRenderDrawColor(renderer, 45, 55, 70, 255);
    SDL_RenderDrawLine(renderer, modal.x + 20, modal.y + 54, modal.x + modal_w - 20, modal.y + 54);

    int cur_y = modal.y + 68;

    // 1. Color Palettes
    draw_text(renderer, "DISPLAY THEMES:", modal.x + 24, cur_y, 1, 255, 200, 50);
    cur_y += 18;

    int pal_w = 200;
    int pal_h = 32;
    for (int i = 0; i < NUM_PALETTES; i++) {
        int px = modal.x + 24 + (i % 3) * (pal_w + 14);
        int py = cur_y + (i / 3) * (pal_h + 10);
        SDL_Rect p_rect = { px, py, pal_w, pal_h };

        bool is_selected = (ctx.config.palette == i);
        draw_button(renderer, p_rect, PALETTES[i].name, is_selected, PALETTES[i].accent[0], PALETTES[i].accent[1], PALETTES[i].accent[2], 1);

        // Swatch previews inside button
        SDL_Rect bg_swatch = { px + 8, py + 8, 14, 16 };
        SDL_Rect fg_swatch = { px + 24, py + 8, 14, 16 };
        SDL_SetRenderDrawColor(renderer, PALETTES[i].bg[0], PALETTES[i].bg[1], PALETTES[i].bg[2], 255);
        SDL_RenderFillRect(renderer, &bg_swatch);
        SDL_SetRenderDrawColor(renderer, PALETTES[i].fg[0], PALETTES[i].fg[1], PALETTES[i].fg[2], 255);
        SDL_RenderFillRect(renderer, &fg_swatch);
    }
    cur_y += 88;

    // 2. Display & Audio Toggles
    draw_text(renderer, "DISPLAY & AUDIO OPTIONS:", modal.x + 24, cur_y, 1, 255, 200, 50);
    cur_y += 18;

    SDL_Rect scan_btn = { modal.x + 24, cur_y, 200, 34 };
    std::string scan_lbl = ctx.config.scanlines_enabled ? "[X] CRT SCANLINES" : "[ ] CRT SCANLINES";
    draw_button(renderer, scan_btn, scan_lbl, false, 0, 220, 255, 1);

    SDL_Rect bezel_btn = { modal.x + 238, cur_y, 200, 34 };
    std::string bezel_lbl = ctx.config.crt_bezel_enabled ? "[X] VINTAGE BEZEL" : "[ ] VINTAGE BEZEL";
    draw_button(renderer, bezel_btn, bezel_lbl, false, 0, 220, 255, 1);

    SDL_Rect mute_toggle = { modal.x + 452, cur_y, 200, 34 };
    std::string mute_lbl = ctx.config.sound_muted ? "[X] SOUND MUTED" : "[ ] SOUND ACTIVE";
    draw_button(renderer, mute_toggle, mute_lbl, false, 0, 220, 255, 1);
    cur_y += 50;

    // 3. Keypad Mapping Diagram
    draw_text(renderer, "CHIP-8 16-KEY HEX KEYPAD MAPPING:", modal.x + 24, cur_y, 1, 255, 200, 50);
    cur_y += 18;

    const char* chip_rows[4] = { "1 2 3 C", "4 5 6 D", "7 8 9 E", "A 0 B F" };
    const char* pc_rows[4]   = { "1 2 3 4", "Q W E R", "A S D F", "Z X C V" };

    for (int r = 0; r < 4; r++) {
        std::string map_line = std::string("CHIP8: [ ") + chip_rows[r] + " ]  -->  KEYBOARD: [ " + pc_rows[r] + " ]";
        draw_text(renderer, map_line, modal.x + 40, cur_y + r * 16, 1, 160, 230, 255);
    }
    cur_y += 74;

    // 4. Bottom Action Buttons
    SDL_Rect rescan_btn = { modal.x + 24, modal.y + modal_h - 48, 160, 34 };
    draw_button(renderer, rescan_btn, "RESCAN ROMS", false, 0, 255, 200, 1);

    SDL_Rect clear_btn = { modal.x + 198, modal.y + modal_h - 48, 160, 34 };
    draw_button(renderer, clear_btn, "CLEAR RECENT", false, 255, 160, 50, 1);

    SDL_Rect close_btn = { modal.x + modal_w - 140, modal.y + modal_h - 48, 116, 34 };
    draw_button(renderer, close_btn, "CLOSE (ESC)", true, 0, 240, 220, 1);
}

void UiRenderer::render_keypad_modal(SDL_Renderer* renderer, int win_w, int win_h) {
    // Backdrop
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_Rect full = { 0, 0, win_w, win_h };
    SDL_RenderFillRect(renderer, &full);

    int modal_w = 480;
    int modal_h = 320;
    SDL_Rect modal = { (win_w - modal_w) / 2, (win_h - modal_h) / 2, modal_w, modal_h };
    draw_glass_panel(renderer, modal, 16, 20, 28, 255, 0, 200, 230, 220);

    draw_text_shadow(renderer, "CONTROLS & SHORTCUTS", modal.x + 24, modal.y + 20, 2, 0, 240, 255);
    SDL_SetRenderDrawColor(renderer, 45, 55, 70, 255);
    SDL_RenderDrawLine(renderer, modal.x + 20, modal.y + 50, modal.x + modal_w - 20, modal.y + 50);

    int cur_y = modal.y + 64;
    draw_text(renderer, "CHIP-8 KEYPAD --> PC KEYBOARD:", modal.x + 24, cur_y, 1, 255, 200, 50);
    cur_y += 18;

    draw_text(renderer, "Row 1: [1][2][3][C]  -->  Keys: [1][2][3][4]", modal.x + 30, cur_y, 1, 160, 230, 255); cur_y += 15;
    draw_text(renderer, "Row 2: [4][5][6][D]  -->  Keys: [Q][W][E][R]", modal.x + 30, cur_y, 1, 160, 230, 255); cur_y += 15;
    draw_text(renderer, "Row 3: [7][8][9][E]  -->  Keys: [A][S][D][F]", modal.x + 30, cur_y, 1, 160, 230, 255); cur_y += 15;
    draw_text(renderer, "Row 4: [A][0][B][F]  -->  Keys: [Z][X][C][V]", modal.x + 30, cur_y, 1, 160, 230, 255); cur_y += 24;

    draw_text(renderer, "HOTKEYS:", modal.x + 24, cur_y, 1, 255, 200, 50); cur_y += 18;
    draw_text(renderer, "SPACE/P: Pause | N: Step | C: Palette | ESC: Exit", modal.x + 30, cur_y, 1, 220, 220, 220); cur_y += 15;
    draw_text(renderer, "F1-F4: Slot | F5: Save | F9: Load | +/-: Speed", modal.x + 30, cur_y, 1, 220, 220, 220); cur_y += 30;

    SDL_Rect close_btn = { modal.x + modal_w / 2 - 60, modal.y + modal_h - 45, 120, 32 };
    draw_button(renderer, close_btn, "OK (ESC)", true, 0, 240, 220, 1);
}

void UiRenderer::render_toast(SDL_Renderer* renderer, const Toast& toast, int win_w) {
    if (!toast.is_active()) return;

    int toast_w = (int)toast.message.length() * 8 + 32;
    int toast_h = 32;
    int toast_x = (win_w - toast_w) / 2;
    int toast_y = 74;

    SDL_Rect t_rect = { toast_x, toast_y, toast_w, toast_h };
    draw_glass_panel(renderer, t_rect, 10, 15, 22, 240, toast.r, toast.g, toast.b, 255);
    draw_text_centered(renderer, toast.message, toast_x + toast_w / 2, toast_y + 11, 1, toast.r, toast.g, toast.b, 255);
}

void UiRenderer::render(SDL_Renderer* renderer, AppContext& ctx, Chip8& chip8, int win_w, int win_h) {
    if (ctx.screen == ScreenMode::LAUNCHER) {
        render_launcher(renderer, ctx, win_w, win_h);
    } else {
        render_gameplay(renderer, ctx, chip8, win_w, win_h);
    }

    if (ctx.show_settings_modal) {
        render_settings_modal(renderer, ctx, win_w, win_h);
    } else if (ctx.show_keypad_modal) {
        render_keypad_modal(renderer, win_w, win_h);
    }

    render_toast(renderer, ctx.toast, win_w);
}

void UiRenderer::handle_mouse_move(AppContext& ctx, int mx, int my, int win_w, int win_h) {
    (void)win_h;
    hovered_tab = -1;
    hovered_card = -1;
    hovered_btn = -1;

    if (ctx.screen == ScreenMode::LAUNCHER) {
        // Check tabs
        int tab_x = 240;
        int tab_w = 100;
        for (int i = 0; i < 5; i++) {
            SDL_Rect t_rect = { tab_x + i * tab_w, 14, tab_w - 8, 36 };
            if (point_in_rect(mx, my, t_rect)) {
                hovered_tab = i;
                break;
            }
        }

        // Rescan and Settings buttons in header
        SDL_Rect rescan_rect = { win_w - 225, 14, 100, 36 };
        if (point_in_rect(mx, my, rescan_rect)) hovered_btn = 1;
        SDL_Rect set_rect = { win_w - 115, 14, 95, 36 };
        if (point_in_rect(mx, my, set_rect)) hovered_btn = 2;

        // Quick play button in hero
        int start_y = 74 - (int)ctx.scroll_y;
        SDL_Rect q_btn = { 30 + (win_w - 60) - 220, start_y + 28, 190, 44 };
        if (point_in_rect(mx, my, q_btn)) hovered_btn = 99;

        // Cards
        int card_w = 280;
        int card_h = 215;
        int gap = 20;
        int cols = std::max(1, (win_w - 60 + gap) / (card_w + gap));
        int grid_y = start_y + 100 + 44;

        for (size_t i = 0; i < ctx.filtered_indices.size(); i++) {
            int col = (int)i % cols;
            int row = (int)i / cols;
            int cx = 30 + col * (card_w + gap);
            int cy = grid_y + row * (card_h + gap);
            SDL_Rect card_rect = { cx, cy, card_w, card_h };
            if (point_in_rect(mx, my, card_rect)) {
                hovered_card = (int)i;
                break;
            }
        }
    } else {
        // Gameplay top bar buttons
        SDL_Rect back_btn = { 16, 9, 120, 32 };
        if (point_in_rect(mx, my, back_btn)) hovered_btn = 10;
        SDL_Rect mute_btn = { win_w - 460, 9, 85, 32 };
        if (point_in_rect(mx, my, mute_btn)) hovered_btn = 11;
        SDL_Rect pal_btn = { win_w - 365, 9, 140, 32 };
        if (point_in_rect(mx, my, pal_btn)) hovered_btn = 12;
        SDL_Rect deb_btn = { win_w - 215, 9, 105, 32 };
        if (point_in_rect(mx, my, deb_btn)) hovered_btn = 13;
        SDL_Rect set_btn = { win_w - 100, 9, 85, 32 };
        if (point_in_rect(mx, my, set_btn)) hovered_btn = 14;

        // Gameplay bottom bar buttons
        SDL_Rect pp_btn = { 20, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, pp_btn)) hovered_btn = 20;
        SDL_Rect rst_btn = { 130, win_h - 48, 95, 36 };
        if (point_in_rect(mx, my, rst_btn)) hovered_btn = 21;
        SDL_Rect spd_dec = { 245, win_h - 48, 30, 36 };
        if (point_in_rect(mx, my, spd_dec)) hovered_btn = 22;
        SDL_Rect spd_inc = { 245 + 175, win_h - 48, 30, 36 };
        if (point_in_rect(mx, my, spd_inc)) hovered_btn = 23;

        // Slot buttons
        int slot_x = 245 + 225 + 45;
        for (int s = 1; s <= 4; s++) {
            SDL_Rect s_btn = { slot_x + (s - 1) * 32, win_h - 48, 28, 36 };
            if (point_in_rect(mx, my, s_btn)) hovered_btn = 30 + s;
        }

        int save_x = slot_x + 140;
        SDL_Rect save_btn = { save_x, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, save_btn)) hovered_btn = 24;
        SDL_Rect load_btn = { save_x + 110, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, load_btn)) hovered_btn = 25;
        if (ctx.paused) {
            SDL_Rect step_btn = { save_x + 220, win_h - 48, 95, 36 };
            if (point_in_rect(mx, my, step_btn)) hovered_btn = 26;
        }
        SDL_Rect key_btn = { win_w - 120, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, key_btn)) hovered_btn = 27;
    }
}

void UiRenderer::handle_mouse_wheel(AppContext& ctx, int wheel_y, int win_h) {
    (void)win_h;
    if (ctx.screen == ScreenMode::LAUNCHER && !ctx.show_settings_modal && !ctx.show_keypad_modal) {
        ctx.scroll_y -= wheel_y * 35.0f;
        ctx.scroll_y = std::clamp(ctx.scroll_y, 0.0f, ctx.max_scroll_y);
    }
}

UiClickResult UiRenderer::handle_mouse_click(AppContext& ctx, Chip8& chip8, int mx, int my, int win_w, int win_h) {
    (void)chip8;
    UiClickResult res;

    // 1. Settings Modal Active
    if (ctx.show_settings_modal) {
        int modal_w = 700;
        int modal_h = 520;
        int modal_x = (win_w - modal_w) / 2;
        int modal_y = (win_h - modal_h) / 2;

        // Theme buttons
        int cur_y = modal_y + 86;
        int pal_w = 200;
        int pal_h = 32;
        for (int i = 0; i < NUM_PALETTES; i++) {
            int px = modal_x + 24 + (i % 3) * (pal_w + 14);
            int py = cur_y + (i / 3) * (pal_h + 10);
            SDL_Rect p_rect = { px, py, pal_w, pal_h };
            if (point_in_rect(mx, my, p_rect)) {
                res.type = UiClickResult::SET_PALETTE;
                res.int_val = i;
                return res;
            }
        }

        // Toggles
        int opt_y = modal_y + 224;
        SDL_Rect scan_btn = { modal_x + 24, opt_y, 200, 34 };
        if (point_in_rect(mx, my, scan_btn)) {
            res.type = UiClickResult::TOGGLE_SCANLINES;
            return res;
        }
        SDL_Rect mute_btn = { modal_x + 452, opt_y, 200, 34 };
        if (point_in_rect(mx, my, mute_btn)) {
            res.type = UiClickResult::TOGGLE_MUTE;
            return res;
        }

        // Actions
        SDL_Rect rescan_btn = { modal_x + 24, modal_y + modal_h - 48, 160, 34 };
        if (point_in_rect(mx, my, rescan_btn)) {
            res.type = UiClickResult::RESCAN_ROMS;
            return res;
        }
        SDL_Rect clear_btn = { modal_x + 198, modal_y + modal_h - 48, 160, 34 };
        if (point_in_rect(mx, my, clear_btn)) {
            res.type = UiClickResult::CLEAR_RECENT;
            return res;
        }
        SDL_Rect close_btn = { modal_x + modal_w - 140, modal_y + modal_h - 48, 116, 34 };
        if (point_in_rect(mx, my, close_btn) || !point_in_rect(mx, my, { modal_x, modal_y, modal_w, modal_h })) {
            res.type = UiClickResult::TOGGLE_SETTINGS;
            return res;
        }
        return res;
    }

    // 2. Keypad Modal Active
    if (ctx.show_keypad_modal) {
        int modal_w = 480;
        int modal_h = 320;
        int modal_x = (win_w - modal_w) / 2;
        int modal_y = (win_h - modal_h) / 2;
        SDL_Rect close_btn = { modal_x + modal_w / 2 - 60, modal_y + modal_h - 45, 120, 32 };
        if (point_in_rect(mx, my, close_btn) || !point_in_rect(mx, my, { modal_x, modal_y, modal_w, modal_h })) {
            res.type = UiClickResult::TOGGLE_KEYPAD;
            return res;
        }
        return res;
    }

    // 3. Launcher Mode
    if (ctx.screen == ScreenMode::LAUNCHER) {
        // Search bar focus
        int search_w = 220;
        int search_x = win_w - search_w - 240;
        SDL_Rect s_rect = { search_x, 14, search_w, 36 };
        ctx.search_focused = point_in_rect(mx, my, s_rect);

        // Header Tabs
        int tab_x = 240;
        int tab_w = 100;
        for (int i = 0; i < 5; i++) {
            SDL_Rect t_rect = { tab_x + i * tab_w, 14, tab_w - 8, 36 };
            if (point_in_rect(mx, my, t_rect)) {
                res.type = UiClickResult::SET_TAB;
                res.int_val = i;
                return res;
            }
        }

        // Rescan Button
        SDL_Rect rescan_rect = { win_w - 225, 14, 100, 36 };
        if (point_in_rect(mx, my, rescan_rect)) {
            res.type = UiClickResult::RESCAN_ROMS;
            return res;
        }

        // Settings Button
        SDL_Rect set_rect = { win_w - 115, 14, 95, 36 };
        if (point_in_rect(mx, my, set_rect)) {
            res.type = UiClickResult::TOGGLE_SETTINGS;
            return res;
        }

        // Quick Launch Button in hero
        int start_y = 74 - (int)ctx.scroll_y;
        SDL_Rect q_btn = { 30 + (win_w - 60) - 220, start_y + 28, 190, 44 };
        if (point_in_rect(mx, my, q_btn) && !ctx.roms.empty()) {
            res.type = UiClickResult::LAUNCH_ROM;
            res.rom_index = 0; // Launch first/featured ROM
            return res;
        }

        // Cards Click
        int card_w = 280;
        int card_h = 215;
        int gap = 20;
        int cols = std::max(1, (win_w - 60 + gap) / (card_w + gap));
        int grid_y = start_y + 100 + 44;

        for (size_t i = 0; i < ctx.filtered_indices.size(); i++) {
            int rom_idx = ctx.filtered_indices[i];
            int col = (int)i % cols;
            int row = (int)i / cols;
            int cx = 30 + col * (card_w + gap);
            int cy = grid_y + row * (card_h + gap);

            // Favorite button click
            SDL_Rect fav_btn = { cx + card_w - 32, cy + 6, 24, 24 };
            if (point_in_rect(mx, my, fav_btn)) {
                res.type = UiClickResult::TOGGLE_FAVORITE;
                res.rom_index = rom_idx;
                return res;
            }

            // Card or Play button click
            SDL_Rect card_rect = { cx, cy, card_w, card_h };
            if (point_in_rect(mx, my, card_rect)) {
                res.type = UiClickResult::LAUNCH_ROM;
                res.rom_index = rom_idx;
                return res;
            }
        }
    } else {
        // 4. Gameplay Mode
        // Back to Library
        SDL_Rect back_btn = { 16, 9, 120, 32 };
        if (point_in_rect(mx, my, back_btn)) {
            res.type = UiClickResult::RETURN_TO_LAUNCHER;
            return res;
        }
        // Mute button
        SDL_Rect mute_btn = { win_w - 460, 9, 85, 32 };
        if (point_in_rect(mx, my, mute_btn)) {
            res.type = UiClickResult::TOGGLE_MUTE;
            return res;
        }
        // Palette cycle button
        SDL_Rect pal_btn = { win_w - 365, 9, 140, 32 };
        if (point_in_rect(mx, my, pal_btn)) {
            res.type = UiClickResult::SET_PALETTE;
            res.int_val = (ctx.config.palette + 1) % NUM_PALETTES;
            return res;
        }
        // Debugger toggle
        SDL_Rect deb_btn = { win_w - 215, 9, 105, 32 };
        if (point_in_rect(mx, my, deb_btn)) {
            res.type = UiClickResult::TOGGLE_DEBUGGER;
            return res;
        }
        // Settings
        SDL_Rect set_btn = { win_w - 100, 9, 85, 32 };
        if (point_in_rect(mx, my, set_btn)) {
            res.type = UiClickResult::TOGGLE_SETTINGS;
            return res;
        }

        // Bottom Bar Controls
        SDL_Rect pp_btn = { 20, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, pp_btn)) {
            res.type = UiClickResult::TOGGLE_PAUSE;
            return res;
        }
        SDL_Rect rst_btn = { 130, win_h - 48, 95, 36 };
        if (point_in_rect(mx, my, rst_btn)) {
            res.type = UiClickResult::RESTART_ROM;
            return res;
        }
        SDL_Rect spd_dec = { 245, win_h - 48, 30, 36 };
        if (point_in_rect(mx, my, spd_dec)) {
            res.type = UiClickResult::SET_SPEED;
            res.int_val = -1;
            return res;
        }
        SDL_Rect spd_inc = { 245 + 175, win_h - 48, 30, 36 };
        if (point_in_rect(mx, my, spd_inc)) {
            res.type = UiClickResult::SET_SPEED;
            res.int_val = +1;
            return res;
        }

        int slot_x = 245 + 225 + 45;
        for (int s = 1; s <= 4; s++) {
            SDL_Rect s_btn = { slot_x + (s - 1) * 32, win_h - 48, 28, 36 };
            if (point_in_rect(mx, my, s_btn)) {
                res.type = UiClickResult::SET_SLOT;
                res.int_val = s;
                return res;
            }
        }

        int save_x = slot_x + 140;
        SDL_Rect save_btn = { save_x, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, save_btn)) {
            res.type = UiClickResult::SAVE_STATE;
            return res;
        }
        SDL_Rect load_btn = { save_x + 110, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, load_btn)) {
            res.type = UiClickResult::LOAD_STATE;
            return res;
        }
        if (ctx.paused) {
            SDL_Rect step_btn = { save_x + 220, win_h - 48, 95, 36 };
            if (point_in_rect(mx, my, step_btn)) {
                res.type = UiClickResult::STEP_INSTRUCTION;
                return res;
            }
        }
        SDL_Rect key_btn = { win_w - 120, win_h - 48, 100, 36 };
        if (point_in_rect(mx, my, key_btn)) {
            res.type = UiClickResult::TOGGLE_KEYPAD;
            return res;
        }
    }

    return res;
}
