// Modèle de jeu pur : plateau, physique des pièces, gravité, délai de
// verrouillage, suppression de lignes, détection des T-spins et score
// "guideline". Aucun rendu ni gestion d'entrée n'ont lieu ici.

#pragma once

#include "Piece.h"
#include "Randomizer.h"

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace tetris {

// État global de la partie.
enum class GameState { Playing, Paused, GameOver };

// Sous-état interne du jeu, piloté par l'horloge du modèle.
enum class Phase { Drop, Landing, Clearing };

// Score "guideline" : 100 / 300 / 500 / 800 points par ligne, x niveau.
int scoreForLines(int linesCleared, int level);

// Bonus T-spin (guideline). `mini` sélectionne la variante mini.
int tSpinScore(bool mini, int linesCleared, int level);

// Bonus de combo (suppressions consécutives) : 50 * combo * niveau.
int comboScore(int combo, int level);

// Bonus de "perfect clear" (plateau vidé).
int perfectClearBonus(int level);

// Modèle de jeu pur : aucun rendu, aucune entrée clavier, aucun accès
// fichier. L'interface terminal est une couche séparée qui lit l'état et
// déclenche des actions.
class Game {
public:
    Game();
    explicit Game(Randomizer randomizer);

    void start();

    // Actions (sans effet si l'état n'est pas "Playing").
    void moveLeft();
    void moveRight();
    void softDrop();
    void hardDrop();
    bool rotateCW();
    bool rotateCCW();
    void holdPiece();
    void togglePause();

    // Gravité, délai de verrouillage et animation de suppression pilotés par
    // le temps. L'interface appelle update() à chaque itération ; advance()
    // est un point d'entrée déterministe réservé aux tests.
    void update(std::chrono::steady_clock::time_point now);
    void advance(std::chrono::milliseconds dt);
    // Millisecondes avant le prochain événement de gravité/verrouillage/
    // suppression (0 si l'événement est dû maintenant).
    long long nextEventInMs() const;

    // Accesseurs d'état utilisés pour le rendu.
    GameState state() const { return state_; }
    Phase phase() const { return phase_; }
    const std::array<std::array<int, kCols>, kRows>& board() const { return board_; }
    // Rangées clignotantes pendant l'animation de suppression (Phase::Clearing).
    const std::vector<int>& clearingRows() const { return clearingRows_; }
    std::optional<Piece> current() const { return current_; }
    std::optional<Piece> ghost() const;
    std::optional<PieceType> hold() const { return hold_; }
    std::optional<PieceType> next() const { return nextPiece_; }
    bool canHold() const { return canHold_; }
    int score() const { return score_; }
    int lines() const { return lines_; }
    int level() const { return level_; }
    int combo() const { return combo_; }
    // Libellé lisible du dernier événement de score (ex. "T-SPIN", "COMBO x3").
    const std::string& lastEvent() const { return lastEvent_; }
    const std::array<int, 4>& lineClears() const { return lineClears_; }
    long long fallIntervalMs() const { return fallIntervalMsForLevel(level_); }
    int durationSeconds() const;

    // Point d'entrée réservé aux tests (inutilisé par le jeu lui-même).
    void setCellForTesting(int row, int col, bool filled);

private:
    bool tryMoveDown(int pointsPerCell);
    bool rotate(int direction);
    bool collides(const Piece& piece) const;
    bool isGrounded() const;
    bool trySpawn(PieceType type);
    PieceType takeNext();
    void prefetchNext();
    void spawnNext();
    void lockCurrent();
    void writeCurrentToBoard();
    // 0 = aucun, 1 = mini, 2 = T-spin complet.
    int detectTSpin() const;
    std::vector<int> findFullRows() const;
    void collapseRows(const std::vector<int>& rows);
    void finishClear();
    void applyScoring(int cleared, int tSpinType);
    bool isPerfectClear() const;
    void reground();
    void setGameOver();
    void advanceInternal(long long dtMs);
    static long long fallIntervalMsForLevel(int level);

    Randomizer randomizer_;
    std::array<std::array<int, kCols>, kRows> board_{};
    std::optional<Piece> current_;
    std::optional<PieceType> hold_;
    bool canHold_ = true;
    std::optional<PieceType> nextPiece_;
    GameState state_ = GameState::GameOver;
    Phase phase_ = Phase::Drop;
    int score_ = 0;
    int lines_ = 0;
    int level_ = 1;
    int combo_ = 0;
    std::array<int, 4> lineClears_{};
    bool backToBack_ = false;
    bool tSpinFlag_ = false;
    std::string lastEvent_;
    std::vector<int> clearingRows_;

    long long nowMs_ = 0;
    long long nextGravityMs_ = 0;
    long long lockDeadlineMs_ = 0;
    long long clearingDeadlineMs_ = 0;
    int lockResets_ = 0;
    std::chrono::steady_clock::time_point lastUpdate_{};
    std::chrono::steady_clock::time_point startedAt_{};
    std::optional<std::chrono::steady_clock::time_point> endedAt_;
};

} // namespace tetris
