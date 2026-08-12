// Persistance du tableau des meilleurs scores (fichier CSV). Charge, enregistre
// et renvoie les meilleures entrées.

#pragma once

#include <string>
#include <vector>

namespace tetris {

// Une entrée du tableau des scores.
struct ScoreEntry {
    std::string pseudo;
    int score = 0;
    int timeSeconds = 0;
    int lines1 = 0;
    int lines2 = 0;
    int lines3 = 0;
    int lines4 = 0;
};

// Charge et enregistre le tableau des meilleurs scores (fichier CSV).
class ScoreStore {
public:
    explicit ScoreStore(std::string file = "scores.txt");

    // Renvoie les scores triés par score, du plus haut au plus bas.
    std::vector<ScoreEntry> load() const;

    // Insère une entrée, re-trie et réécrit le fichier.
    void save(const ScoreEntry& entry);

    // Les `n` meilleurs scores, ou moins s'il n'y en a pas assez.
    std::vector<ScoreEntry> top(int n) const;

private:
    // Analyse une ligne CSV ; `ok` reçoit false si la ligne est invalide.
    static ScoreEntry parseLine(const std::string& line, bool* ok);

    std::string file_;
};

} // namespace tetris
