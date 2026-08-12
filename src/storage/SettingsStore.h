#pragma once

#include <string>

namespace tetris {

// Ghost-piece color codes, mirroring ncurses COLOR_* values (kept here so
// the storage layer stays ncurses-free).
constexpr int kGhostColorBlack = 0;
constexpr int kGhostColorRed = 1;
constexpr int kGhostColorGreen = 2;
constexpr int kGhostColorYellow = 3;
constexpr int kGhostColorBlue = 4;
constexpr int kGhostColorMagenta = 5;
constexpr int kGhostColorCyan = 6;
constexpr int kGhostColorWhite = 7;

// User-configurable gameplay settings. The arrow keys always work as an
// alternative to the secondary keys shown in the help bar. `ghostColor` is
// one of the kGhostColor* codes above.
struct Settings {
    int moveLeft = 'j';
    int moveRight = 'l';
    int rotate = 'k';
    int softDrop = 's';
    int hardDrop = ' ';
    int ghostColor = kGhostColorBlack;
};

// Loads and persists the settings file (one "name,value" pair per line).
class SettingsStore {
public:
    explicit SettingsStore(std::string file = "settings.txt");

    // Reads the file; missing or invalid entries keep their defaults.
    Settings load() const;

    // Writes the current settings back to the file.
    void save(const Settings& settings) const;

private:
    std::string file_;
};

} // namespace tetris
