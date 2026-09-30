#include "app_state.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

const Palette PALETTES[6] = {
    {"Classic Green",   {  0,  20,   0}, { 51, 255,  51}, {100, 255, 100}},
    {"Amber CRT",       { 24,  12,   0}, {255, 176,   0}, {255, 210,  50}},
    {"Neon Cyberpunk",  { 10,   6,  26}, {  0, 255, 240}, {255,   0, 128}},
    {"White Arcade",    { 10,  10,  14}, {245, 245, 250}, {100, 180, 255}},
    {"Game Boy 1989",   { 15,  56,  15}, {155, 188,  15}, {139, 172,  15}},
    {"Solarized Ruby",  { 24,   5,  12}, {255,  60,  90}, {255, 140, 160}}
};
const int NUM_PALETTES = sizeof(PALETTES) / sizeof(PALETTES[0]);

bool AppConfig::save(const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "palette=" << palette << "\n";
    out << "cycles_per_frame=" << cycles_per_frame << "\n";
    out << "scanlines=" << (scanlines_enabled ? 1 : 0) << "\n";
    out << "crt_bezel=" << (crt_bezel_enabled ? 1 : 0) << "\n";
    out << "muted=" << (sound_muted ? 1 : 0) << "\n";
    out << "slot=" << savestate_slot << "\n";

    out << "favorites=";
    bool first = true;
    for (const auto& fav : favorites) {
        if (!first) out << ",";
        out << fav;
        first = false;
    }
    out << "\n";

    out << "recent=";
    first = true;
    for (const auto& r : recent_roms) {
        if (!first) out << ",";
        out << r;
        first = false;
    }
    out << "\n";

    return true;
}

bool AppConfig::load(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if (key == "palette") palette = std::clamp(std::stoi(val), 0, NUM_PALETTES - 1);
            else if (key == "cycles_per_frame") cycles_per_frame = std::clamp(std::stoi(val), 1, 100);
            else if (key == "scanlines") scanlines_enabled = (std::stoi(val) != 0);
            else if (key == "crt_bezel") crt_bezel_enabled = (std::stoi(val) != 0);
            else if (key == "muted") sound_muted = (std::stoi(val) != 0);
            else if (key == "slot") savestate_slot = std::clamp(std::stoi(val), 1, 4);
            else if (key == "favorites") {
                favorites.clear();
                std::stringstream ss(val);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    if (!item.empty()) favorites.insert(item);
                }
            } else if (key == "recent") {
                recent_roms.clear();
                std::stringstream ss(val);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    if (!item.empty()) recent_roms.push_back(item);
                }
            }
        } catch (...) {}
    }
    return true;
}

AppContext::AppContext() {}

void AppContext::init() {
    config.load("chip8_config.ini");
    scan_roms("roms");
    update_filter();
}

static uint32_t hash_str(const std::string& str) {
    uint32_t h = 2166136261u;
    for (char c : str) {
        h = (h ^ (uint8_t)c) * 16777619u;
    }
    return h;
}

static std::string to_lower_str(const std::string& s) {
    std::string res = s;
    for (char& c : res) c = (char)std::tolower((unsigned char)c);
    return res;
}

void AppContext::scan_roms(const std::string& root_dir) {
    roms.clear();
    std::string dir_to_search = root_dir;
    if (!std::filesystem::exists(dir_to_search)) {
        if (std::filesystem::exists("roms")) dir_to_search = "roms";
        else return;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_to_search)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ch8") {
                RomItem item;
                item.path = entry.path().lexically_normal().string();
                // Normalize slashes to forward slashes for cross-platform consistency
                for (char& c : item.path) if (c == '\\') c = '/';
                item.filename = entry.path().filename().string();
                item.file_size = (size_t)entry.file_size();
                item.seed = hash_str(item.path);

                std::string lower_fn = to_lower_str(item.filename);

                // Determine Titles & Descriptions
                if (lower_fn.find("pong") != std::string::npos) {
                    item.title = "Pong";
                    item.category = "Sports / Arcade";
                    item.description = "Classic 1970s two-player paddle rally arcade action.";
                    item.art_type = ProceduralArtType::PONG;
                } else if (lower_fn.find("tetris") != std::string::npos) {
                    item.title = "Tetris";
                    item.category = "Puzzle / Classic";
                    item.description = "Stack, rotate, and clear falling tetromino lines.";
                    item.art_type = ProceduralArtType::TETRIS;
                } else if (lower_fn.find("blinky") != std::string::npos) {
                    item.title = "Blinky";
                    item.category = "Maze / Arcade";
                    item.description = "Chomp through dot-filled mazes while evading ghosts.";
                    item.art_type = ProceduralArtType::BLINKY;
                } else if (lower_fn.find("chip8-logo") != std::string::npos) {
                    item.title = "CHIP-8 Logo Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Timendus benchmark displaying standard CHIP-8 logo.";
                    item.art_type = ProceduralArtType::LOGO;
                } else if (lower_fn.find("ibm-logo") != std::string::npos) {
                    item.title = "IBM Logo Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Timendus ROM rendering official IBM emblem.";
                    item.art_type = ProceduralArtType::LOGO;
                } else if (lower_fn.find("corax") != std::string::npos) {
                    item.title = "Corax+ Opcode Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Exhaustive opcode integrity check by Corax.";
                    item.art_type = ProceduralArtType::TEST;
                } else if (lower_fn.find("flags") != std::string::npos) {
                    item.title = "Flags Arithmetic Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Timendus test verifying carry, borrow, and VF ordering.";
                    item.art_type = ProceduralArtType::TEST;
                } else if (lower_fn.find("quirks") != std::string::npos) {
                    item.title = "Quirks Verification Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Evaluates legacy COSMAC VIP vs SCHIP platform quirks.";
                    item.art_type = ProceduralArtType::TEST;
                } else if (lower_fn.find("keypad") != std::string::npos) {
                    item.title = "Keypad Input Test";
                    item.category = "Test & Diagnostic";
                    item.description = "Real-time visual diagnostic for 16 hex input keys.";
                    item.art_type = ProceduralArtType::TEST;
                } else {
                    // Clean fallback title from filename
                    std::string stem = entry.path().stem().string();
                    std::string clean;
                    for (size_t i = 0; i < stem.size(); i++) {
                        char c = stem[i];
                        if (c == '-' || c == '_') clean += ' ';
                        else clean += c;
                    }
                    item.title = clean;
                    if (item.path.find("tests") != std::string::npos) {
                        item.category = "Test & Diagnostic";
                        item.art_type = ProceduralArtType::TEST;
                    } else {
                        item.category = "Classic Arcade";
                        item.art_type = ProceduralArtType::GENERIC;
                    }
                    item.description = "CHIP-8 ROM binary (" + std::to_string(item.file_size) + " bytes).";
                }

                item.is_favorite = (config.favorites.find(item.path) != config.favorites.end());
                roms.push_back(item);
            }
        }
    } catch (...) {}

    // Sort: Classic games first, tests second, then alphabetical
    std::sort(roms.begin(), roms.end(), [](const RomItem& a, const RomItem& b) {
        bool a_is_test = (a.category == "Test & Diagnostic");
        bool b_is_test = (b.category == "Test & Diagnostic");
        if (a_is_test != b_is_test) return !a_is_test; // games before tests
        return a.title < b.title;
    });

    update_filter();
}

void AppContext::update_filter() {
    filtered_indices.clear();
    std::string query = to_lower_str(search_query);

    for (size_t i = 0; i < roms.size(); i++) {
        const auto& r = roms[i];

        // Tab filter
        bool tab_match = false;
        switch (tab) {
            case TabFilter::ALL: tab_match = true; break;
            case TabFilter::FAVORITES: tab_match = r.is_favorite; break;
            case TabFilter::RECENT: {
                for (const auto& rec : config.recent_roms) {
                    if (rec == r.path) { tab_match = true; break; }
                }
                break;
            }
            case TabFilter::GAMES: tab_match = (r.category != "Test & Diagnostic"); break;
            case TabFilter::TESTS: tab_match = (r.category == "Test & Diagnostic"); break;
        }

        if (!tab_match) continue;

        // Search query filter
        if (!query.empty()) {
            std::string t = to_lower_str(r.title);
            std::string c = to_lower_str(r.category);
            std::string fn = to_lower_str(r.filename);
            if (t.find(query) == std::string::npos &&
                c.find(query) == std::string::npos &&
                fn.find(query) == std::string::npos) {
                continue;
            }
        }

        filtered_indices.push_back((int)i);
    }
}

void AppContext::toggle_favorite(int rom_index) {
    if (rom_index < 0 || rom_index >= (int)roms.size()) return;
    auto& r = roms[rom_index];
    r.is_favorite = !r.is_favorite;
    if (r.is_favorite) {
        config.favorites.insert(r.path);
        show_toast("Added to Favorites: " + r.title, 255, 215, 0);
    } else {
        config.favorites.erase(r.path);
        show_toast("Removed from Favorites: " + r.title, 180, 180, 180);
    }
    config.save();
    update_filter();
}

void AppContext::record_played(int rom_index) {
    if (rom_index < 0 || rom_index >= (int)roms.size()) return;
    const auto& path = roms[rom_index].path;
    // Remove if already in recent list
    auto it = std::remove(config.recent_roms.begin(), config.recent_roms.end(), path);
    config.recent_roms.erase(it, config.recent_roms.end());
    // Insert at front
    config.recent_roms.insert(config.recent_roms.begin(), path);
    // Keep max 10 recent
    if (config.recent_roms.size() > 10) config.recent_roms.resize(10);
    config.save();
}

void AppContext::show_toast(const std::string& message, uint8_t r, uint8_t g, uint8_t b, uint32_t duration) {
    toast.message = message;
    toast.start_time = SDL_GetTicks();
    toast.duration_ms = duration;
    toast.r = r;
    toast.g = g;
    toast.b = b;
}

RomItem* AppContext::get_selected_rom() {
    if (selected_rom_index >= 0 && selected_rom_index < (int)roms.size()) {
        return &roms[selected_rom_index];
    }
    return nullptr;
}
