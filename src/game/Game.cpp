// Implémentation du modèle de jeu Game.

#include "Game.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace tetris {

namespace {
constexpr long long kLockDelayMs = 500;   // délai de verrouillage (500 ms)
constexpr int kLockResetsMax = 15;        // réinitialisations max du délai
constexpr long long kClearFlashMs = 300;  // durée du clignotement de lignes
} // namespace

// Score d'une suppression de `linesCleared` lignes (1 à 4), x niveau.
int scoreForLines(int linesCleared, int level) {
    static const int kScores[5] = {0, 100, 300, 500, 800};
    if (linesCleared < 1 || linesCleared > 4) {
        return 0;
    }
    return kScores[linesCleared] * level;
}

// Score d'un T-spin (mini ou complet), en fonction des lignes supprimées.
int tSpinScore(bool mini, int linesCleared, int level) {
    if (mini) {
        static const int kMini[2] = {100, 200};
        if (linesCleared < 0 || linesCleared > 1) return 0;
        return kMini[linesCleared] * level;
    }
    static const int kNormal[4] = {400, 800, 1200, 1600};
    if (linesCleared < 0 || linesCleared > 3) return 0;
    return kNormal[linesCleared] * level;
}

// Score d'un combo : 50 points par niveau de combo, x niveau.
int comboScore(int combo, int level) {
    return 50 * combo * level;
}

// Bonus de "perfect clear" : plateau entièrement vidé.
int perfectClearBonus(int level) {
    return 3500 * level;
}

// Constructeur par défaut : utilise un générateur aléatoire.
Game::Game() = default;

// Constructeur avec un générateur injecté (séquence fixe pour les tests).
Game::Game(Randomizer randomizer) : randomizer_(std::move(randomizer)) {}

// Démarre une nouvelle partie : réinitialise le plateau, le score et tous
// les compteurs, puis fait apparaître la première pièce.
void Game::start() {
    board_.fill({});
    score_ = 0;
    lines_ = 0;
    level_ = 1;
    combo_ = 0;
    lineClears_.fill(0);
    backToBack_ = false;
    tSpinFlag_ = false;
    canHold_ = true;
    hold_.reset();
    nextPiece_.reset();
    current_.reset();
    endedAt_.reset();
    clearingRows_.clear();
    lastEvent_.clear();
    nowMs_ = 0;
    nextGravityMs_ = 0;
    lockDeadlineMs_ = 0;
    clearingDeadlineMs_ = 0;
    lockResets_ = 0;
    phase_ = Phase::Drop;
    state_ = GameState::Playing;
    startedAt_ = std::chrono::steady_clock::now();
    lastUpdate_ = startedAt_;
    spawnNext();
}

// Vérifie si la pièce donnée chevauche un mur, le sol ou une cellule déjà
// remplie du plateau.
bool Game::collides(const Piece& piece) const {
    for (auto [dr, dc] : occupiedCells(piece.type, piece.rotation)) {
        const int r = piece.row + dr;
        const int c = piece.col + dc;
        if (c < 0 || c >= kCols) return true;
        if (r >= kRows) return true;
        if (r >= 0 &&
            board_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != 0) {
            return true;
        }
    }
    return false;
}

// Indique si la pièce courante ne peut plus descendre d'une case.
bool Game::isGrounded() const {
    if (!current_) return false;
    Piece down = *current_;
    ++down.row;
    return collides(down);
}

// Fait apparaître une pièce en haut du plateau. Renvoie false (et termine
// la partie) si le spawn chevauche déjà des blocs.
bool Game::trySpawn(PieceType type) {
    if (state_ != GameState::Playing) return false;
    Piece p;
    p.type = type;
    p.rotation = 0;
    p.row = 0;
    p.col = (kCols - pieceSize(type)) / 2;
    if (collides(p)) {
        setGameOver();
        return false;
    }
    current_ = p;
    return true;
}

// Consomme la pièce "next" pré-affichée, sinon en tire une du générateur.
PieceType Game::takeNext() {
    if (nextPiece_) {
        PieceType t = *nextPiece_;
        nextPiece_.reset();
        return t;
    }
    return randomizer_.next();
}

// Tire à l'avance la pièce suivante si elle n'est pas encore connue.
void Game::prefetchNext() {
    if (!nextPiece_) {
        nextPiece_ = randomizer_.next();
    }
}

// Fait apparaître la pièce suivante et planifie la prochaine chute de
// gravité.
void Game::spawnNext() {
    phase_ = Phase::Drop;
    canHold_ = true;
    prefetchNext();
    if (!trySpawn(takeNext())) return;
    prefetchNext();
    nextGravityMs_ = nowMs_ + fallIntervalMs();
}

// Écrit les cellules de la pièce courante dans le plateau (avec la couleur
// encodée comme type + 1).
void Game::writeCurrentToBoard() {
    if (!current_) return;
    for (auto [dr, dc] : occupiedCells(current_->type, current_->rotation)) {
        const int r = current_->row + dr;
        const int c = current_->col + dc;
        if (r >= 0 && r < kRows && c >= 0 && c < kCols) {
            board_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] =
                static_cast<int>(current_->type) + 1;
        }
    }
}

// Cherche les rangées complètes (du bas vers le haut).
std::vector<int> Game::findFullRows() const {
    std::vector<int> rows;
    for (int r = kRows - 1; r >= 0; --r) {
        const auto& row = board_[static_cast<std::size_t>(r)];
        if (std::all_of(row.begin(), row.end(),
                        [](int cell) { return cell != 0; })) {
            rows.push_back(r);
        }
    }
    return rows;
}

// Supprime les rangées indiquées en faisant glisser les blocs au-dessus
// vers le bas.
void Game::collapseRows(const std::vector<int>& rows) {
    if (rows.empty()) return;
    std::vector<bool> removed(kRows, false);
    for (int r : rows) {
        removed[static_cast<std::size_t>(r)] = true;
    }
    int write = kRows - 1;
    for (int r = kRows - 1; r >= 0; --r) {
        if (!removed[static_cast<std::size_t>(r)]) {
            board_[static_cast<std::size_t>(write--)] =
                board_[static_cast<std::size_t>(r)];
        }
    }
    for (int r = write; r >= 0; --r) {
        board_[static_cast<std::size_t>(r)].fill(0);
    }
}

// Vrai si le plateau est vide hors des rangées en cours de suppression.
bool Game::isPerfectClear() const {
    for (int r = 0; r < kRows; ++r) {
        if (std::find(clearingRows_.begin(), clearingRows_.end(), r) !=
            clearingRows_.end()) {
            continue;
        }
        for (int c = 0; c < kCols; ++c) {
            if (board_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != 0) {
                return false;
            }
        }
    }
    return true;
}

// Détecte un T-spin (règle des trois coins) pour la pièce T courante :
// renvoie 2 (complet) si au moins 3 coins sont occupés, 1 (mini) si 2 coins.
int Game::detectTSpin() const {
    if (!current_ || current_->type != PieceType::T || !tSpinFlag_) return 0;
    static const int kOffsets[4][2] = {{0, 0}, {0, 2}, {2, 0}, {2, 2}};
    int corners = 0;
    for (const auto& off : kOffsets) {
        const int r = current_->row + off[0];
        const int c = current_->col + off[1];
        if (r < 0 || r >= kRows || c < 0 || c >= kCols ||
            board_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != 0) {
            ++corners;
        }
    }
    if (corners >= 3) return 2;
    if (corners == 2) return 1;
    return 0;
}

// Applique le score d'une suppression de lignes et/ou d'un T-spin : points
// de base, combos, bonus back-to-back, perfect clear, et mise à jour du
// niveau ainsi que du libellé du dernier événement.
void Game::applyScoring(int cleared, int tSpinType) {
    int gained = 0;
    lastEvent_.clear();
    const int counted = std::min(cleared, 4);
    if (cleared > 0) {
        gained += scoreForLines(counted, level_);
        if (tSpinType == 2) gained += tSpinScore(false, counted, level_);
        else if (tSpinType == 1) gained += tSpinScore(true, counted, level_);
        ++combo_;
        gained += comboScore(combo_, level_);
        const bool isSpecial = tSpinType == 2 || counted == 4;
        const bool b2bBonus = isSpecial && backToBack_;
        if (b2bBonus) gained = gained * 3 / 2;
        backToBack_ = isSpecial;
        if (isPerfectClear()) gained += perfectClearBonus(level_);
        lines_ += counted;
        lineClears_[static_cast<std::size_t>(counted - 1)]++;
        level_ = lines_ / 10 + 1;

        if (tSpinType == 2) {
            lastEvent_ = (counted == 1)   ? "T-SPIN SINGLE"
                         : (counted == 2) ? "T-SPIN DOUBLE"
                                          : "T-SPIN TRIPLE";
        } else if (tSpinType == 1) {
            lastEvent_ = "MINI T-SPIN";
        } else if (counted == 4) {
            lastEvent_ = "TETRIS";
        } else {
            lastEvent_ = std::to_string(counted) + " LINES";
        }
        if (b2bBonus) lastEvent_ += " x1.5";
        if (combo_ > 1) lastEvent_ += " COMBO x" + std::to_string(combo_);
        if (isPerfectClear()) lastEvent_ += " PERFECT CLEAR!";
    } else {
        combo_ = 0;
        backToBack_ = false;
        if (tSpinType == 2) {
            gained += tSpinScore(false, 0, level_);
            lastEvent_ = "T-SPIN";
        } else if (tSpinType == 1) {
            gained += tSpinScore(true, 0, level_);
            lastEvent_ = "MINI T-SPIN";
        }
    }
    score_ += gained;
}

// Verrouille la pièce courante dans le plateau, puis lance la suppression
// des lignes pleines (avec animation) ou fait apparaître la pièce suivante.
void Game::lockCurrent() {
    if (!current_ || state_ != GameState::Playing) return;
    writeCurrentToBoard();
    const int tSpinType = detectTSpin();
    const std::vector<int> full = findFullRows();
    tSpinFlag_ = false;
    current_.reset();
    if (!full.empty()) {
        clearingRows_ = full;
        phase_ = Phase::Clearing;
        clearingDeadlineMs_ = nowMs_ + kClearFlashMs;
        applyScoring(static_cast<int>(full.size()), tSpinType);
        return;
    }
    applyScoring(0, tSpinType);
    spawnNext();
}

// Termine l'animation de suppression : efface les rangées pleines puis
// fait apparaître la pièce suivante.
void Game::finishClear() {
    collapseRows(clearingRows_);
    clearingRows_.clear();
    spawnNext();
}

// Termine la partie : enregistre l'heure de fin et retire la pièce courante.
void Game::setGameOver() {
    state_ = GameState::GameOver;
    endedAt_ = std::chrono::steady_clock::now();
    current_.reset();
    phase_ = Phase::Drop;
}

// Fait descendre la pièce d'une case et ajoute les points demandés (1 pour
// un soft drop, 2 par case pour un hard drop). Passe en phase Landing quand
// la pièce touche le sol.
bool Game::tryMoveDown(int pointsPerCell) {
    if (state_ != GameState::Playing || !current_) return false;
    Piece down = *current_;
    ++down.row;
    if (collides(down)) {
        if (phase_ != Phase::Landing) {
            phase_ = Phase::Landing;
            lockDeadlineMs_ = nowMs_ + kLockDelayMs;
            lockResets_ = 0;
        }
        return false;
    }
    current_ = down;
    score_ += pointsPerCell;
    tSpinFlag_ = false;
    if (phase_ == Phase::Landing) {
        phase_ = Phase::Drop;
        nextGravityMs_ = nowMs_ + fallIntervalMs();
    }
    return true;
}

// Réévalue l'état d'appui au sol : relance le délai de verrouillage (avec
// réinitialisations limitées) ou revient à la chute libre après un déplacement.
void Game::reground() {
    if (!current_ || state_ != GameState::Playing) return;
    if (isGrounded()) {
        if (phase_ == Phase::Landing) {
            if (lockResets_ < kLockResetsMax) {
                lockDeadlineMs_ = nowMs_ + kLockDelayMs;
                ++lockResets_;
            }
        } else {
            phase_ = Phase::Landing;
            lockDeadlineMs_ = nowMs_ + kLockDelayMs;
            lockResets_ = 0;
        }
    } else if (phase_ == Phase::Landing) {
        phase_ = Phase::Drop;
        nextGravityMs_ = nowMs_ + fallIntervalMs();
    }
}

// Déplace la pièce d'une colonne vers la gauche si possible.
void Game::moveLeft() {
    if (state_ != GameState::Playing || !current_) return;
    Piece moved = *current_;
    --moved.col;
    if (!collides(moved)) {
        current_ = moved;
        tSpinFlag_ = false;
        reground();
    }
}

// Déplace la pièce d'une colonne vers la droite si possible.
void Game::moveRight() {
    if (state_ != GameState::Playing || !current_) return;
    Piece moved = *current_;
    ++moved.col;
    if (!collides(moved)) {
        current_ = moved;
        tSpinFlag_ = false;
        reground();
    }
}

// Soft drop : une case vers le bas, 1 point par case.
void Game::softDrop() {
    tryMoveDown(1);
}

// Hard drop : la pièce tombe immédiatement jusqu'au sol, 2 points par case,
// puis se verrouille sans délai.
void Game::hardDrop() {
    if (state_ != GameState::Playing || !current_) return;
    int cells = 0;
    Piece g = *current_;
    while (true) {
        Piece down = g;
        ++down.row;
        if (collides(down)) break;
        g = down;
        ++cells;
    }
    score_ += 2 * cells;
    current_ = g;
    tSpinFlag_ = false;
    lockCurrent();
}

// Rotation horaire (CW).
bool Game::rotateCW() {
    return rotate(1);
}

// Rotation anti-horaire (CCW).
bool Game::rotateCCW() {
    return rotate(3);
}

// Tente une rotation de `direction` pas (1 = CW, 3 = CCW) en essayant les
// décalages de wall-kick SRS. Active le drapeau de T-spin si la pièce est
// posée après la rotation.
bool Game::rotate(int direction) {
    if (state_ != GameState::Playing || !current_) return false;
    if (current_->type == PieceType::O) return true;
    const int from = current_->rotation;
    const int to = (from + direction) % 4;
    for (auto [dr, dc] : kickOffsets(current_->type, from, direction == 1)) {
        Piece candidate = *current_;
        candidate.rotation = to;
        candidate.row += dr;
        candidate.col += dc;
        if (!collides(candidate)) {
            current_ = candidate;
            if (isGrounded()) tSpinFlag_ = true;
            reground();
            return true;
        }
    }
    return false;
}

// Échange la pièce courante avec la pièce en réserve (une seule fois par
// pièce, sauf si la réserve vient d'être vidée).
void Game::holdPiece() {
    if (state_ != GameState::Playing || !current_ || !canHold_) return;
    const PieceType held = current_->type;
    PieceType incoming;
    if (hold_) {
        incoming = *hold_;
    } else {
        prefetchNext();
        incoming = takeNext();
    }
    if (!trySpawn(incoming)) return;
    hold_ = held;
    canHold_ = false;
    tSpinFlag_ = false;
    phase_ = Phase::Drop;
    nextGravityMs_ = nowMs_ + fallIntervalMs();
    prefetchNext();
}

// Bascule entre les états Playing et Paused.
void Game::togglePause() {
    if (state_ == GameState::Playing) {
        state_ = GameState::Paused;
    } else if (state_ == GameState::Paused) {
        state_ = GameState::Playing;
    }
}

// Avance la simulation de `dtMs` millisecondes : gère l'animation de
// suppression, le délai de verrouillage et la gravité.
void Game::advanceInternal(long long dtMs) {
    if (state_ != GameState::Playing) return;
    nowMs_ += dtMs;
    if (phase_ == Phase::Clearing) {
        if (nowMs_ >= clearingDeadlineMs_) finishClear();
        return;
    }
    if (phase_ == Phase::Landing) {
        if (nowMs_ >= lockDeadlineMs_) lockCurrent();
        return;
    }
    while (phase_ == Phase::Drop && nowMs_ >= nextGravityMs_) {
        if (tryMoveDown(0)) {
            nextGravityMs_ += fallIntervalMs();
        } else {
            break;
        }
    }
}

// Point d'entrée déterministe pour les tests : avance le temps d'un delta.
void Game::advance(std::chrono::milliseconds dt) {
    advanceInternal(dt.count());
}

// Avance le jeu en fonction de l'horloge réelle (bornée à 200 ms par appel
// pour éviter les grands sauts lors d'un ralentissement de l'interface).
void Game::update(std::chrono::steady_clock::time_point now) {
    if (state_ != GameState::Playing) {
        lastUpdate_ = now;
        return;
    }
    if (lastUpdate_ == std::chrono::steady_clock::time_point{}) {
        lastUpdate_ = now;
    }
    long long dt =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate_)
            .count();
    lastUpdate_ = now;
    if (dt < 0) dt = 0;
    advanceInternal(std::min(dt, 200LL));
}

// Nombre de millisecondes avant le prochain événement du jeu ; l'interface
// l'utilise pour calibrer son délai d'attente sur les entrées.
long long Game::nextEventInMs() const {
    if (state_ != GameState::Playing) return 500;
    if (phase_ == Phase::Clearing) {
        return std::max(0LL, clearingDeadlineMs_ - nowMs_);
    }
    if (phase_ == Phase::Landing) {
        return std::max(0LL, lockDeadlineMs_ - nowMs_);
    }
    return std::max(0LL, nextGravityMs_ - nowMs_);
}

// Position de la "pièce fantôme" : la pièce courante projetée tout en bas
// du plateau.
std::optional<Piece> Game::ghost() const {
    if (state_ != GameState::Playing || !current_) return std::nullopt;
    Piece g = *current_;
    while (true) {
        Piece down = g;
        ++down.row;
        if (collides(down)) break;
        g = down;
    }
    return g;
}

// Intervalle de chute (ms) pour un niveau donné : diminue à mesure que le
// niveau augmente, avec une accélération "guideline".
long long Game::fallIntervalMsForLevel(int level) {
    double secs = std::pow(0.8 - (level - 1) * 0.007, level - 1);
    if (level >= 20) {
        secs = 0.06 - (level - 20) * 0.005;
    }
    secs = std::clamp(secs, 0.05, 0.8);
    return static_cast<long long>(secs * 1000.0);
}

// Durée totale de la partie en secondes (jusqu'à la fin ou à maintenant).
int Game::durationSeconds() const {
    if (startedAt_ == std::chrono::steady_clock::time_point{}) return 0;
    const auto end = endedAt_ ? *endedAt_ : std::chrono::steady_clock::now();
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                end - startedAt_)
                                .count());
}

// Remplit ou vide une cellule du plateau (réservé aux tests).
void Game::setCellForTesting(int row, int col, bool filled) {
    if (row < 0 || row >= kRows || col < 0 || col >= kCols) return;
    board_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] =
        filled ? 1 : 0;
}

} // namespace tetris
