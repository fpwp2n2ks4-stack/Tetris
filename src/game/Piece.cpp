// Implémentation des fonctions d'aide aux pièces déclarées dans Piece.h.

#include "Piece.h"

#include <cstddef>
#include <utility>

namespace tetris {

// Renvoie la grille d'occupation (rotation 0) du type de pièce donné.
// Chaque cellule vaut 1 si elle est occupée, 0 sinon.
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

// Renvoie la taille (n x n) de la boîte englobante de la pièce.
int pieceSize(PieceType type) {
    return static_cast<int>(baseShape(type).size());
}

namespace {

// Calcule la grille d'occupation de la pièce après `rotation` tours de 90°
// dans le sens horaire (les rotations négatives sont normalisées).
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

// Tables de wall-kick SRS : décalages (rangée, colonne) à tester pour
// chaque rotation source, dans le sens horaire (CW) ou anti-horaire (CCW).
// L'ordre correspond aux lignes officielles du SRS.
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
// Tables de kick vides pour la pièce O, qui ne se déplace jamais en rotant.
const std::vector<std::pair<int, int>> kEmpty;

} // namespace

// Énumère les cellules occupées (décalages relatifs) après rotation.
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

// Renvoie la liste de décalages de wall-kick à essayer pour la rotation
// `from` -> `from + 1` (CW) ou `from - 1` (CCW), selon le type de pièce.
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
