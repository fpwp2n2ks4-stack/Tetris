// Implémentation du générateur de pièces 7-bag.

#include "Randomizer.h"

#include <algorithm>
#include <random>
#include <utility>

namespace tetris {

// Constructeur par défaut : grain aléatoire (horloge du système).
Randomizer::Randomizer() : Randomizer(std::random_device{}()) {}

// Constructeur déterministe : les sacs mélangés seront toujours identiques
// pour une même graine, ce qui rend les tests reproductibles.
Randomizer::Randomizer(uint32_t seed) : gen_(seed) {
    index_ = bag_.size();
}

// Constructeur "séquence fixe" : sert une séquence imposée en boucle, sans
// jamais utiliser le générateur aléatoire.
Randomizer::Randomizer(std::vector<PieceType> sequence)
    : fixed_(std::move(sequence)) {}

// Construit un nouveau sac contenant une fois chaque pièce et le mélange.
void Randomizer::refill() {
    bag_ = {
        PieceType::I, PieceType::O, PieceType::T,
        PieceType::S, PieceType::Z, PieceType::J,
        PieceType::L,
    };
    std::shuffle(bag_.begin(), bag_.end(), gen_);
    index_ = 0;
}

// Renvoie la pièce suivante : depuis la séquence fixe si elle est définie,
// sinon depuis le sac courant (reconstruit quand il est épuisé).
PieceType Randomizer::next() {
    if (!fixed_.empty()) {
        PieceType p = fixed_[fixedIndex_ % fixed_.size()];
        ++fixedIndex_;
        return p;
    }
    if (index_ >= bag_.size()) {
        refill();
    }
    return bag_[index_++];
}

} // namespace tetris
