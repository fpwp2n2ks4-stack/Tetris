// Implémentation de la persistance des meilleurs scores.

#include "ScoreStore.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tetris {

namespace {

// Nettoie un pseudo : retire les virgules et les espaces de début/fin.
std::string cleanPseudo(std::string pseudo) {
    for (char& ch : pseudo) {
        if (ch == ',' || ch == '\n' || ch == '\r') ch = ' ';
    }
    const auto notSpace = [](unsigned char c) { return c != ' '; };
    const auto first = std::find_if(pseudo.begin(), pseudo.end(), notSpace);
    if (first == pseudo.end()) return "Anonymous";
    const auto last = std::find_if(pseudo.rbegin(), pseudo.rend(), notSpace).base();
    return std::string(first, last);
}

} // namespace

// Constructeur : choisit le fichier qui contient le tableau des scores.
ScoreStore::ScoreStore(std::string file) : file_(std::move(file)) {}

// Analyse une ligne CSV "pseudo,score,temps,1L,2L,3L,4L" et renseigne
// `ok` selon la validité de la ligne.
ScoreEntry ScoreStore::parseLine(const std::string& line, bool* ok) {
    ScoreEntry entry;
    std::vector<std::string> parts;
    std::istringstream iss(line);
    std::string token;
    while (std::getline(iss, token, ',')) {
        parts.push_back(token);
    }
    if (parts.size() < 7) {
        if (ok) *ok = false;
        return entry;
    }
    try {
        entry.pseudo = cleanPseudo(parts[0]);
        entry.score = std::stoi(parts[1]);
        entry.timeSeconds = std::stoi(parts[2]);
        entry.lines1 = std::stoi(parts[3]);
        entry.lines2 = std::stoi(parts[4]);
        entry.lines3 = std::stoi(parts[5]);
        entry.lines4 = std::stoi(parts[6]);
    } catch (const std::exception&) {
        if (ok) *ok = false;
        return entry;
    }
    if (ok) *ok = true;
    return entry;
}

// Charge toutes les entrées valides du fichier.
std::vector<ScoreEntry> ScoreStore::load() const {
    std::vector<ScoreEntry> scores;
    std::ifstream in(file_);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        bool ok = false;
        ScoreEntry entry = parseLine(line, &ok);
        if (ok) scores.push_back(entry);
    }
    return scores;
}

// Insère une nouvelle entrée, trie les scores par ordre décroissant et
// réécrit le fichier complet.
void ScoreStore::save(const ScoreEntry& entry) {
    auto scores = load();
    scores.push_back(entry);
    std::sort(scores.begin(), scores.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) {
                  return a.score > b.score;
              });
    std::ofstream out(file_, std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& s : scores) {
        out << cleanPseudo(s.pseudo) << "," << s.score << "," << s.timeSeconds
            << "," << s.lines1 << "," << s.lines2 << "," << s.lines3 << ","
            << s.lines4 << "\n";
    }
}

// Renvoie les `n` meilleurs scores (ou tous s'il y en a moins).
std::vector<ScoreEntry> ScoreStore::top(int n) const {
    auto scores = load();
    if (static_cast<int>(scores.size()) > n) {
        scores.resize(static_cast<size_t>(n));
    }
    return scores;
}

} // namespace tetris
