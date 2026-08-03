#pragma once

#include "Piece.h"

#include <array>
#include <random>
#include <vector>

namespace tetris {

// 7-bag randomizer: draws from a shuffled bag of the 7 tetrominoes so that
// no piece appears twice before every other piece has appeared.
class Randomizer {
public:
    Randomizer();

    // Deterministic 7-bag for tests.
    explicit Randomizer(uint32_t seed);

    // Fixed sequence that cycles forever; used by tests.
    explicit Randomizer(std::vector<PieceType> sequence);

    PieceType next();

private:
    void refill();

    std::vector<PieceType> fixed_;
    size_t fixedIndex_ = 0;
    std::array<PieceType, kPieceCount> bag_{};
    size_t index_ = bag_.size();
    std::mt19937 gen_;
};

} // namespace tetris
