#pragma once

#include <utility>
#include <vector>

namespace tetris {

constexpr int kRows = 20;
constexpr int kCols = 10;

enum class PieceType : int {
    I = 0,
    O = 1,
    T = 2,
    S = 3,
    Z = 4,
    J = 5,
    L = 6,
};

constexpr int kPieceCount = 7;

struct Piece {
    PieceType type = PieceType::I;
    int rotation = 0; // 0..3
    int row = 0;
    int col = 0;
};

// Side length of the piece's bounding box (2, 3 or 4).
int pieceSize(PieceType type);

// Rotation-0 occupancy grid.
const std::vector<std::vector<int>>& baseShape(PieceType type);

// Occupied cells relative to the piece's top-left corner.
std::vector<std::pair<int, int>> occupiedCells(PieceType type, int rotation);

// SRS wall-kick offsets (row, col) to try when rotating from state `from`.
// `clockwise` selects the CW or CCW kick table for that transition.
const std::vector<std::pair<int, int>>& kickOffsets(PieceType type, int from, bool clockwise);

} // namespace tetris
