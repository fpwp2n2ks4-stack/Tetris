#include "Randomizer.h"

#include <algorithm>
#include <random>
#include <utility>

namespace tetris {

Randomizer::Randomizer() : Randomizer(std::random_device{}()) {}

Randomizer::Randomizer(uint32_t seed) : gen_(seed) {
    index_ = bag_.size();
}

Randomizer::Randomizer(std::vector<PieceType> sequence)
    : fixed_(std::move(sequence)) {}

void Randomizer::refill() {
    bag_ = {
        PieceType::I, PieceType::O, PieceType::T,
        PieceType::S, PieceType::Z, PieceType::J,
        PieceType::L,
    };
    std::shuffle(bag_.begin(), bag_.end(), gen_);
    index_ = 0;
}

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
