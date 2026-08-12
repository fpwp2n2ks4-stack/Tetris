// Définitions des pièces : les sept tétrominos, la taille de leur boîte
// englobante, leur grille d'occupation à la rotation 0, ainsi que les tables
// de wall-kick SRS. Aucune donnée d'état n'est stockée ici.

#pragma once

#include <utility>
#include <vector>

namespace tetris {

// Taille de la grille de jeu (20 rangées x 10 colonnes).
constexpr int kRows = 20;
constexpr int kCols = 10;

// Les sept types de pièces du tétromino standard.
enum class PieceType : int {
    I = 0,
    O = 1,
    T = 2,
    S = 3,
    Z = 4,
    J = 5,
    L = 6,
};

// Nombre de types de pièces distincts.
constexpr int kPieceCount = 7;

// Position et orientation courantes d'une pièce sur le plateau.
struct Piece {
    PieceType type = PieceType::I;
    int rotation = 0; // 0..3
    int row = 0;
    int col = 0;
};

// Côté de la boîte englobante de la pièce (2, 3 ou 4).
int pieceSize(PieceType type);

// Grille d'occupation à la rotation 0.
const std::vector<std::vector<int>>& baseShape(PieceType type);

// Cellules occupées, relatives au coin supérieur gauche de la pièce.
std::vector<std::pair<int, int>> occupiedCells(PieceType type, int rotation);

// Décalages de wall-kick SRS (rangée, colonne) à essayer lors d'une rotation
// depuis l'état `from`. `clockwise` sélectionne la table CW ou CCW.
const std::vector<std::pair<int, int>>& kickOffsets(PieceType type, int from, bool clockwise);

} // namespace tetris
