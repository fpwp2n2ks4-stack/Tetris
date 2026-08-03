#pragma once

#include <string>
#include <vector>

namespace tetris {

struct ScoreEntry {
    std::string pseudo;
    int score = 0;
    int timeSeconds = 0;
    int lines1 = 0;
    int lines2 = 0;
    int lines3 = 0;
    int lines4 = 0;
};

// Loads and persists the high-score table (CSV file).
class ScoreStore {
public:
    explicit ScoreStore(std::string file = "scores.txt");

    // Returns scores sorted by score, highest first.
    std::vector<ScoreEntry> load() const;

    // Inserts an entry, re-sorts and writes the file back.
    void save(const ScoreEntry& entry);

    // Top `n` scores, or fewer if there are not enough entries.
    std::vector<ScoreEntry> top(int n) const;

private:
    static ScoreEntry parseLine(const std::string& line, bool* ok);

    std::string file_;
};

} // namespace tetris
