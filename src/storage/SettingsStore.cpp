#include "SettingsStore.h"

#include <fstream>
#include <string>
#include <utility>

namespace tetris {

namespace {

constexpr const char* kLeft = "left";
constexpr const char* kRight = "right";
constexpr const char* kRotate = "rotate";
constexpr const char* kSoftDrop = "softdrop";
constexpr const char* kHardDrop = "harddrop";
constexpr const char* kGhostColor = "ghostcolor";

} // namespace

SettingsStore::SettingsStore(std::string file) : file_(std::move(file)) {}

Settings SettingsStore::load() const {
    Settings settings;
    std::ifstream in(file_);
    if (!in.is_open()) return settings;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        const auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        const std::string name = line.substr(0, comma);
        const std::string value = line.substr(comma + 1);
        try {
            const int code = std::stoi(value);
            if (name == kLeft) {
                settings.moveLeft = code;
            } else if (name == kRight) {
                settings.moveRight = code;
            } else if (name == kRotate) {
                settings.rotate = code;
            } else if (name == kSoftDrop) {
                settings.softDrop = code;
            } else if (name == kHardDrop) {
                settings.hardDrop = code;
            } else if (name == kGhostColor) {
                settings.ghostColor = code & 0x7;
            }
        } catch (const std::exception&) {
            // Ignore malformed lines.
        }
    }
    return settings;
}

void SettingsStore::save(const Settings& settings) const {
    std::ofstream out(file_, std::ios::trunc);
    if (!out.is_open()) return;
    out << kLeft << "," << settings.moveLeft << "\n"
        << kRight << "," << settings.moveRight << "\n"
        << kRotate << "," << settings.rotate << "\n"
        << kSoftDrop << "," << settings.softDrop << "\n"
        << kHardDrop << "," << settings.hardDrop << "\n"
        << kGhostColor << "," << settings.ghostColor << "\n";
}

} // namespace tetris
