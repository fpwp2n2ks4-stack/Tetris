#pragma once

#include "../game/Game.h"
#include "../storage/ScoreStore.h"

#include <optional>
#include <string>

namespace tetris {

// Ncurses front-end: renders the model state and maps keyboard input to
// Game actions. Contains no game rules.
class NcursesUi {
public:
    NcursesUi() = default;
    ~NcursesUi();

    NcursesUi(const NcursesUi&) = delete;
    NcursesUi& operator=(const NcursesUi&) = delete;

    bool init();
    void shutdown();

    // Returns 0 = play, 1 = high scores, 2 = quit.
    int showMainMenu();

    // Runs the game loop. Returns true if the game ended, false if the
    // player quit mid-game (no score is saved in that case).
    bool playGame(Game& game);

    // Final stats screen; returns the player's name.
    std::string promptPseudo(const Game& game);

    void showHighScores(const ScoreStore& store);

private:
    static int colorId(PieceType type);
    static std::string formatTime(int seconds);

    void renderGame(const Game& game) const;
    void printCell(int id) const;
    void printMiniLine(const std::optional<PieceType>& piece, int row) const;
    void printSidePanel(const Game& game, int row) const;

    bool initialized_ = false;
    bool showNextPiece_ = true;
};

} // namespace tetris
