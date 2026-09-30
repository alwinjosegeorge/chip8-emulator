#ifndef UI_FONT_H
#define UI_FONT_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cmath>

// 8x8 Bitmap Font for ASCII 32 (' ') through 126 ('~')
// 8 rows per glyph, 1 byte per row (MSB = leftmost pixel)
extern const uint8_t FONT_8X8[95][8];

inline void draw_char8x8(SDL_Renderer* renderer, char c, int x, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* glyph = FONT_8X8[c - 32];
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    for (int row = 0; row < 8; row++) {
        uint8_t byte = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                if (scale == 1) {
                    SDL_RenderDrawPoint(renderer, x + col, y + row);
                } else {
                    SDL_Rect rect = { x + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
    }
}

inline int get_text_width(const std::string& text, int scale) {
    return (int)text.length() * 8 * scale;
}

inline void draw_text(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    int cur_x = x;
    for (char c : text) {
        if (c == '\n') {
            cur_x = x;
            y += 9 * scale;
            continue;
        }
        draw_char8x8(renderer, c, cur_x, y, scale, r, g, b, a);
        cur_x += 8 * scale;
    }
}

inline void draw_text_shadow(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    draw_text(renderer, text, x + scale, y + scale, scale, 0, 0, 0, 160);
    draw_text(renderer, text, x, y, scale, r, g, b, a);
}

inline void draw_text_centered(SDL_Renderer* renderer, const std::string& text, int cx, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    int w = get_text_width(text, scale);
    draw_text(renderer, text, cx - w / 2, y, scale, r, g, b, a);
}

inline void draw_text_right(SDL_Renderer* renderer, const std::string& text, int rx, int y, int scale, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    int w = get_text_width(text, scale);
    draw_text(renderer, text, rx - w, y, scale, r, g, b, a);
}

// Geometric vector-style UI icons drawn directly through SDL2
// (Avoids reliance on Unicode glyphs which may not render in standard fonts)

// 1. Play Icon (Right-pointing filled triangle)
inline void draw_icon_play(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int half = size / 2;
    int left = cx - half / 2;
    int right = cx + half;
    int top = cy - half;
    int bottom = cy + half;
    for (int y = top; y <= bottom; y++) {
        float t = (float)(y - top) / (float)(bottom - top);
        float width_factor = (t <= 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
        int cur_right = left + (int)((right - left) * width_factor);
        SDL_RenderDrawLine(renderer, left, y, cur_right, y);
    }
}

// 2. Pause Icon (Two vertical bars)
inline void draw_icon_pause(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int bar_w = std::max(2, size / 4);
    int bar_h = size;
    int gap = std::max(2, size / 5);
    SDL_Rect r1 = { cx - bar_w - gap / 2, cy - bar_h / 2, bar_w, bar_h };
    SDL_Rect r2 = { cx + gap / 2, cy - bar_h / 2, bar_w, bar_h };
    SDL_RenderFillRect(renderer, &r1);
    SDL_RenderFillRect(renderer, &r2);
}

// 3. Step Icon (Play triangle + vertical stop bar)
inline void draw_icon_step(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    draw_icon_play(renderer, cx - size / 4, cy, size * 3 / 4, r, g, b, a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int bar_w = std::max(2, size / 6);
    SDL_Rect bar = { cx + size / 3, cy - size / 2, bar_w, size };
    SDL_RenderFillRect(renderer, &bar);
}

// 4. Back Arrow Icon
inline void draw_icon_back(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int half = size / 2;
    // Arrow stem
    SDL_Rect stem = { cx - half / 2, cy - 1, half + 3, 3 };
    SDL_RenderFillRect(renderer, &stem);
    // Arrow head
    for (int i = 0; i <= half; i++) {
        SDL_RenderDrawLine(renderer, cx - half + i, cy - i, cx - half + i, cy + i);
    }
}

// 5. Heart / Favorite Icon
inline void draw_icon_heart(SDL_Renderer* renderer, int cx, int cy, int size, bool filled, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    // 8x8 pixel heart pattern centered
    static const uint8_t HEART_MAP[8] = {
        0b01100110,
        0b11111111,
        0b11111111,
        0b11111111,
        0b01111110,
        0b00111100,
        0b00011000,
        0b00000000
    };
    static const uint8_t HEART_OUTLINE[8] = {
        0b01100110,
        0b10011001,
        0b10000001,
        0b10000001,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00000000
    };
    const uint8_t* map = filled ? HEART_MAP : HEART_OUTLINE;
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = map[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 6. Star Icon
inline void draw_icon_star(SDL_Renderer* renderer, int cx, int cy, int size, bool filled, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t STAR_MAP[8] = {
        0b00011000,
        0b00011000,
        0b11111111,
        0b01111110,
        0b00111100,
        0b01100110,
        0b11000011,
        0b10000001
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = STAR_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
    (void)filled;
}

// 7. Search Icon (Magnifying Glass)
inline void draw_icon_search(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t SEARCH_MAP[8] = {
        0b00111100,
        0b01000010,
        0b10000001,
        0b10000001,
        0b01000010,
        0b00111110,
        0b00000111,
        0b00000011
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = SEARCH_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 8. Settings Gear Icon
inline void draw_icon_gear(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t GEAR_MAP[8] = {
        0b00111100,
        0b10111101,
        0b11000011,
        0b11000011,
        0b11000011,
        0b11000011,
        0b10111101,
        0b00111100
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = GEAR_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 9. Restart / Refresh Icon (Circular arrow)
inline void draw_icon_restart(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t RESTART_MAP[8] = {
        0b00111100,
        0b01000010,
        0b10001111,
        0b10000001,
        0b10000001,
        0b11110001,
        0b01000010,
        0b00111100
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = RESTART_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 10. Debugger Bug Icon
inline void draw_icon_bug(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t BUG_MAP[8] = {
        0b01000010,
        0b00111100,
        0b11111111,
        0b00111100,
        0b11111111,
        0b00111100,
        0b01111110,
        0b10000001
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = BUG_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 11. Floppy Save/Disk Icon
inline void draw_icon_floppy(SDL_Renderer* renderer, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t FLOPPY_MAP[8] = {
        0b11111110,
        0b10000011,
        0b10000011,
        0b11111111,
        0b11000011,
        0b11000011,
        0b11000011,
        0b11111111
    };
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = FLOPPY_MAP[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

// 12. Volume / Speaker Icon
inline void draw_icon_volume(SDL_Renderer* renderer, int cx, int cy, int size, bool muted, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    static const uint8_t VOL_MAP[8] = {
        0b00010000,
        0b00110100,
        0b01110010,
        0b11110001,
        0b11110001,
        0b01110010,
        0b00110100,
        0b00010000
    };
    static const uint8_t MUTE_MAP[8] = {
        0b00010000,
        0b00110101,
        0b01110010,
        0b11110101,
        0b11110101,
        0b01110010,
        0b00110101,
        0b00010000
    };
    const uint8_t* map = muted ? MUTE_MAP : VOL_MAP;
    int scale = std::max(1, size / 8);
    int top_x = cx - 4 * scale;
    int top_y = cy - 4 * scale;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = map[row];
        for (int col = 0; col < 8; col++) {
            if ((byte & (0x80 >> col)) != 0) {
                SDL_Rect px = { top_x + col * scale, top_y + row * scale, scale, scale };
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

#endif // UI_FONT_H
