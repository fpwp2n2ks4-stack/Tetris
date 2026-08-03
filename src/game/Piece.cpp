#include "Piece.h"

#include <cstddef>
#include <utility>

namespace tetris {

const std::vector<std::vector<int>>& baseShape(PieceType type) {
    static const std::vector<std::vector<int>> kShapes[kPieceCount] = {
        // I (4x4)
        {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
        // O (2x2)
        {{1, 1}, {1, 1}},
        // T
        {{0, 1, 0}, {1, 1, 1}, {0, 0, 0}},
        // S
        {{0, 1, 1}, {1, 1, 0}, {0, 0, 0}},
        // Z
        {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}},
        // J
        {{1, 0, 0}, {1, 1, 1}, {0, 0, 0}},
        // L
        {{0, 0, 1}, {1, 1, 1}, {0, 0, 0}},
    };
    return kShapes[static_cast<int>(type)];
}

int pieceSize(PieceType type) {
    return static_cast<int>(baseShape(type).size());
}

namespace {

std::vector<std::vector<int>> rotatedShape(PieceType type, int rotation) {
    auto shape = baseShape(type);
    const std::size_t n = shape.size();
    const int rot = ((rotation % 4) + 4) % 4;
    for (int k = 0; k < rot; ++k) {
        std::vector<std::vector<int>> out(n, std::vector<int>(n, 0));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                out[j][n - 1 - i] = shape[i][j];
            }
        }
        shape = std::move(out);
    }
    return shape;
}

const std::vector<std::pair<int, int>> kKicksJlstzCW[4] = {
    {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
    {{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
    {{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
    {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
};
const std::vector<std::pair<int, int>> kKicksJlstzCCW[4] = {
    {{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
    {{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
    {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
    {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
};
const std::vector<std::pair<int, int>> kKicksICW[4] = {
    {{0, 0}, {0, -2}, {0, 1}, {1, -2}, {-2, 1}},
    {{0, 0}, {0, -1}, {0, 2}, {-2, -1}, {1, 2}},
    {{0, 0}, {0, 2}, {0, -1}, {-1, 2}, {2, -1}},
    {{0, 0}, {0, 1}, {0, -2}, {2, -1}, {1, -2}},
};
const std::vector<std::pair<int, int>> kKicksICCW[4] = {
    {{0, 0}, {0, -1}, {0, 2}, {-2, -1}, {1, 2}},
    {{0, 0}, {0, 2}, {0, -1}, {-1, 2}, {2, -1}},
    {{0, 0}, {0, 1}, {0, -2}, {2, -1}, {1, -2}},
    {{0, 0}, {0, -2}, {0, 1}, {1, -2}, {-2, 1}},
};
const std::vector<std::pair<int, int>> kEmpty;

} // namespace

std::vector<std::pair<int, int>> occupiedCells(PieceType type, int rotation) {
    auto shape = rotatedShape(type, rotation);
    std::vector<std::pair<int, int>> cells;
    for (std::size_t i = 0; i < shape.size(); ++i) {
        for (std::size_t j = 0; j < shape[i].size(); ++j) {
            if (shape[i][j]) {
                cells.emplace_back(static_cast<int>(i), static_cast<int>(j));
            }
        }
    }
    return cells;
}

const std::vector<std::pair<int, int>>& kickOffsets(PieceType type, int from, bool clockwise) {
    const int idx = ((from % 4) + 4) % 4;
    if (type == PieceType::I) {
        return clockwise ? kKicksICW[idx] : kKicksICCW[idx];
    }
    if (type == PieceType::O) {
        return kEmpty;
    }
    return clockwise ? kKicksJlstzCW[idx] : kKicksJlstzCCW[idx];
}

} // namespace tetris
