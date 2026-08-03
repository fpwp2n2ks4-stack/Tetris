#pragma once

#include "Piece.h"
#include "Randomizer.h"

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace tetris {

enum class GameState { Playing, Paused, GameOver };

// Internal play sub-state, driven by the model clock.
enum class Phase { Drop, Landing, Clearing };

// Guideline scoring: 100 / 300 / 500 / 800 points per clear, scaled by level.
int scoreForLines(int linesCleared, int level);

// T-spin bonuses (guideline). `mini` selects the mini variant.
int tSpinScore(bool mini, int linesCleared, int level);

// Consecutive line-clear bonus: 50 * combo * level.
int comboScore(int combo, int level);

// Perfect-clear (empty board) bonus.
int perfectClearBonus(int level);

// Pure game model: no rendering, no input handling, no file I/O.
// The terminal UI is a separate layer that only reads state and calls actions.
class Game {
public:
    Game();
    explicit Game(Randomizer randomizer);

    void start();

    // Actions (no-ops unless state is Playing).
    void moveLeft();
    void moveRight();
    void softDrop();
    void hardDrop();
    bool rotateCW();
    bool rotateCCW();
    void holdPiece();
    void togglePause();

    // Time-driven gravity, lock delay and clear animation. The UI calls
    // update() each loop iteration; advance() is a deterministic test hook.
    void update(std::chrono::steady_clock::time_point now);
    void advance(std::chrono::milliseconds dt);
    // Milliseconds until the next gravity/lock/clear event (0 if due now).
    long long nextEventInMs() const;

    // State accessors used for rendering.
    GameState state() const { return state_; }
    Phase phase() const { return phase_; }
    const std::array<std::array<int, kCols>, kRows>& board() const { return board_; }
    // Rows being flashed during the clear animation (Phase::Clearing only).
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
    // Human-readable label of the last scoring event (e.g. "T-SPIN", "COMBO x3").
    const std::string& lastEvent() const { return lastEvent_; }
    const std::array<int, 4>& lineClears() const { return lineClears_; }
    long long fallIntervalMs() const { return fallIntervalMsForLevel(level_); }
    int durationSeconds() const;

    // Test hook (not used by the game itself).
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
    // 0 = none, 1 = mini, 2 = full T-spin.
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
