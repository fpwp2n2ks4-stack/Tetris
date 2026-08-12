// Générateur de pièces "7-bag" : on mélange un sac contenant les sept
// tétrominos, puis on les distribue un par un, garantissant qu'aucune pièce
// n'apparaît deux fois avant que les six autres soient sorties.

#pragma once

#include "Piece.h"

#include <array>
#include <random>
#include <vector>

namespace tetris {

// Générateur de pièces basé sur le principe du 7-bag.
class Randomizer {
public:
    // Sac aléatoire (grain tiré au hasard).
    Randomizer();

    // Sac déterministe pour les tests, à partir d'une graine donnée.
    explicit Randomizer(uint32_t seed);

    // Séquence fixe qui se répète à l'infini ; utilisée par les tests.
    explicit Randomizer(std::vector<PieceType> sequence);

    // Renvoie la prochaine pièce du sac (ou de la séquence fixe).
    PieceType next();

private:
    // Reconstruit et mélange un nouveau sac de sept pièces.
    void refill();

    std::vector<PieceType> fixed_;
    size_t fixedIndex_ = 0;
    std::array<PieceType, kPieceCount> bag_{};
    size_t index_ = bag_.size();
    std::mt19937 gen_;
};

} // namespace tetris
