#include "../src/game/Game.h"
#include "../src/game/Randomizer.h"
#include "../src/storage/ScoreStore.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <utility>

using namespace tetris;
using namespace std::chrono;

static int failures = 0;

#define CHECK(...)                                                             \
    do {                                                                       \
        if (!(__VA_ARGS__)) {                                                  \
            ++failures;                                                        \
            std::cerr << "FAIL at line " << __LINE__ << ": " << #__VA_ARGS__   \
                      << "\n";                                                 \
        }                                                                      \
    } while (0)

using CellSet = std::set<std::pair<int, int>>;

static CellSet cells(PieceType type, int rotation) {
    auto list = occupiedCells(type, rotation);
    return CellSet(list.begin(), list.end());
}

static void testRandomizerSevenBag() {
    Randomizer r(12345);
    for (int bag = 0; bag < 3; ++bag) {
        std::set<PieceType> seen;
        for (int i = 0; i < 7; ++i) seen.insert(r.next());
        CHECK(seen.size() == 7);
    }
}

static void testRotationsAllPieces() {
    // Rotation 0..3 expected occupancy for every piece.
    const CellSet kI[4] = {
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
    };
    const CellSet kO[4] = {
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
        {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
    };
    const CellSet kT[4] = {
        {{0, 1}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 1}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 1}},
    };
    const CellSet kS[4] = {
        {{0, 1}, {0, 2}, {1, 0}, {1, 1}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 1}, {1, 2}, {2, 0}, {2, 1}},
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
    };
    const CellSet kZ[4] = {
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
        {{0, 2}, {1, 1}, {1, 2}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
    };
    const CellSet kJ[4] = {
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {0, 2}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 0}, {2, 1}},
    };
    const CellSet kL[4] = {
        {{0, 2}, {1, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 0}},
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
    };
    const CellSet* all[7] = {kI, kO, kT, kS, kZ, kJ, kL};
    for (int t = 0; t < 7; ++t) {
        const PieceType type = static_cast<PieceType>(t);
        for (int rot = 0; rot < 4; ++rot) {
            CHECK(cells(type, rot) == all[t][rot]);
        }
    }
}

static void testWallKickI() {
    Game game(Randomizer({PieceType::I}));
    game.start();
    CHECK(game.current().has_value());
    for (int i = 0; i < 10; ++i) game.moveRight();
    CHECK(game.current()->col == 6);
    CHECK(game.rotateCW());
    CHECK(game.current()->rotation == 1);
    game.moveRight();
    game.moveRight();
    CHECK(game.current()->col == 7);
    CHECK(game.rotateCCW());
    CHECK(game.current()->rotation == 0);
    CHECK(game.current()->col == 6);
}

static void fillRows(Game& game, int firstRow, int lastRow) {
    for (int r = firstRow; r <= lastRow; ++r) {
        for (int c = 0; c < kCols; ++c) {
            game.setCellForTesting(r, c, true);
        }
    }
}

// Number of rows the current piece would fall before locking.
static int fallCells(const Game& game) {
    if (!game.current()) return 0;
    auto gh = game.ghost();
    return gh ? gh->row - game.current()->row : 0;
}

static void testLineClearAndScoring() {
    Game game(Randomizer({PieceType::O, PieceType::O, PieceType::O}));
    game.start();
    CHECK(game.state() == GameState::Playing);
    fillRows(game, kRows - 1, kRows - 1);
    game.hardDrop();
    CHECK(game.phase() == Phase::Clearing);
    CHECK(game.lines() == 1);
    CHECK(game.score() >= scoreForLines(1, 1));
    CHECK(game.board()[kRows - 2][0] == 0);
    CHECK(game.board()[kRows - 1][4] != 0);
    game.advance(milliseconds(400));
    CHECK(game.phase() == Phase::Drop);
    CHECK(game.current().has_value());
    CHECK(game.board()[kRows - 1][4] != 0);
}

static void testMultiLineClears() {
    {
        Game game(Randomizer({PieceType::O, PieceType::O}));
        game.start();
        fillRows(game, kRows - 2, kRows - 1);
        game.hardDrop();
        CHECK(game.lines() == 2);
        CHECK(game.score() >= scoreForLines(2, 1));
    }
    {
        Game game(Randomizer({PieceType::O, PieceType::O}));
        game.start();
        fillRows(game, kRows - 3, kRows - 1);
        game.hardDrop();
        CHECK(game.lines() == 3);
    }
    {
        Game game(Randomizer({PieceType::O, PieceType::O}));
        game.start();
        fillRows(game, kRows - 4, kRows - 1);
        game.hardDrop();
        CHECK(game.lines() == 4);
        CHECK(game.lineClears()[3] == 1);
    }
}

static void testBackToBack() {
    Game game(Randomizer({PieceType::O, PieceType::O, PieceType::O}));
    game.start();
    fillRows(game, kRows - 4, kRows - 1);
    game.hardDrop(); // tetris #1
    CHECK(game.lines() == 4);
    CHECK(game.lastEvent().find("TETRIS") != std::string::npos);
    const int afterFirst = game.score();
    game.advance(milliseconds(400)); // finish clear animation
    CHECK(game.phase() == Phase::Drop);

    fillRows(game, kRows - 4, kRows - 1);
    const int drop2 = fallCells(game);
    game.hardDrop(); // tetris #2 (back-to-back)
    const int delta = game.score() - afterFirst;
    const int expected =
        2 * drop2 + (scoreForLines(4, 1) + comboScore(2, 1)) * 3 / 2;
    CHECK(delta == expected);
    CHECK(game.lastEvent().find("x1.5") != std::string::npos);
    CHECK(game.lines() == 8);
}

static void testPerfectClear() {
    Game game(Randomizer({PieceType::O, PieceType::O}));
    game.start();
    for (int r = kRows - 2; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (c == 4 || c == 5) continue; // the O piece's landing spot
            game.setCellForTesting(r, c, true);
        }
    }
    const int drop = fallCells(game);
    game.hardDrop(); // fills rows kRows-2 and kRows-1 -> perfect clear
    CHECK(game.lines() == 2);
    CHECK(game.lastEvent().find("PERFECT CLEAR!") != std::string::npos);
    CHECK(game.score() == 2 * drop + scoreForLines(2, 1) + comboScore(1, 1) +
                              perfectClearBonus(1));
    game.advance(milliseconds(400));
    bool empty = true;
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (game.board()[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != 0) {
                empty = false;
            }
        }
    }
    CHECK(empty);
}

static void testTSpin() {
    Game game(Randomizer({PieceType::T, PieceType::I}));
    game.start();
    for (int c = 0; c < kCols; ++c) {
        game.setCellForTesting(kRows - 1, c, false);
    }
    game.setCellForTesting(kRows - 1, 7, true);
    game.setCellForTesting(kRows - 1, 9, true);
    game.moveRight();
    game.moveRight();
    game.moveRight();
    game.moveRight();
    CHECK(game.current()->col == 7);
    for (int i = 0; i < 30; ++i) {
        game.advance(milliseconds(1000));
        if (game.phase() == Phase::Landing) break;
    }
    CHECK(game.phase() == Phase::Landing);
    CHECK(game.current()->rotation == 0);
    CHECK(game.current()->row == kRows - 3);
    game.setCellForTesting(kRows - 3, 7, true); // third filled corner
    CHECK(game.rotateCW());
    CHECK(game.current()->rotation == 1);
    game.advance(milliseconds(600));
    CHECK(game.lines() == 0);
    CHECK(game.score() == tSpinScore(false, 0, 1));
    CHECK(game.lastEvent() == "T-SPIN");
    CHECK(game.current().has_value());
}

static void testLockDelay() {
    Game game(Randomizer({PieceType::I, PieceType::I}));
    game.start();
    for (int i = 0; i < 30; ++i) {
        game.advance(milliseconds(1000));
        if (game.phase() == Phase::Landing) break;
    }
    CHECK(game.phase() == Phase::Landing);
    const int row = game.current()->row;
    CHECK(row > 0);

    game.advance(milliseconds(400)); // still within the lock window
    CHECK(game.phase() == Phase::Landing);

    game.moveLeft(); // resets the lock timer
    game.advance(milliseconds(200));
    CHECK(game.phase() == Phase::Landing); // would have locked without reset
    CHECK(game.current()->row == row);

    game.advance(milliseconds(400));
    CHECK(game.phase() == Phase::Drop); // locked and respawned
    CHECK(game.current().has_value());
    CHECK(game.lines() == 0);
}

static void testHardDropLocksImmediately() {
    Game game(Randomizer({PieceType::I, PieceType::I}));
    game.start();
    game.hardDrop();
    CHECK(game.phase() == Phase::Drop);
    CHECK(game.current()->type == PieceType::I);
    CHECK(game.board()[kRows - 1][4] != 0);
    CHECK(game.board()[0][0] == 0);
}

static void testPause() {
    Game game(Randomizer({PieceType::I, PieceType::I}));
    game.start();
    game.togglePause();
    CHECK(game.state() == GameState::Paused);
    game.advance(milliseconds(10000));
    CHECK(game.current()->row == 0); // gravity frozen while paused
    game.togglePause();
    CHECK(game.state() == GameState::Playing);
}

static void testGhost() {
    Game game(Randomizer({PieceType::I, PieceType::I}));
    game.start();
    auto g = game.ghost();
    CHECK(g.has_value());
    CHECK(g->row == kRows - 2); // horizontal I rests one row above the floor
    CHECK(g->row >= game.current()->row);
}

static void testGameOver() {
    Game game(Randomizer({PieceType::I, PieceType::I}));
    game.start();
    for (int c = 3; c <= 6; ++c) {
        game.setCellForTesting(1, c, true);
    }
    game.hardDrop();
    CHECK(game.state() == GameState::GameOver);
}

static void testHold() {
    Game game(Randomizer({PieceType::I, PieceType::O, PieceType::T}));
    game.start();
    CHECK(game.current()->type == PieceType::I);
    game.holdPiece();
    CHECK(game.hold() == PieceType::I);
    CHECK(game.current()->type == PieceType::O);
    CHECK(!game.canHold());
    game.holdPiece();
    CHECK(game.current()->type == PieceType::O);
    game.hardDrop();
    CHECK(game.state() == GameState::Playing);
    CHECK(game.current()->type == PieceType::T);
    game.holdPiece();
    CHECK(game.hold() == PieceType::T);
    CHECK(game.current()->type == PieceType::I);
}

static void testScoringFunctions() {
    CHECK(scoreForLines(1, 1) == 100);
    CHECK(scoreForLines(2, 1) == 300);
    CHECK(scoreForLines(3, 1) == 500);
    CHECK(scoreForLines(4, 1) == 800);
    CHECK(scoreForLines(1, 3) == 300);
    CHECK(tSpinScore(false, 0, 1) == 400);
    CHECK(tSpinScore(false, 1, 1) == 800);
    CHECK(tSpinScore(false, 2, 1) == 1200);
    CHECK(tSpinScore(false, 3, 1) == 1600);
    CHECK(tSpinScore(true, 0, 1) == 100);
    CHECK(tSpinScore(true, 1, 1) == 200);
    CHECK(tSpinScore(false, 1, 2) == 1600);
    CHECK(comboScore(1, 1) == 50);
    CHECK(comboScore(3, 2) == 300);
    CHECK(perfectClearBonus(1) == 3500);
}

static void testScoreStore() {
    const std::string path = "scores_test.txt";
    {
        std::ofstream out(path);
        out << "Alice,100,60,1,0,0,0\n";
        out << "this,is,not,valid,line\n";
        out << "Bob,200,30,0,1,0,0\n";
    }
    ScoreStore store(path);
    auto loaded = store.load();
    CHECK(loaded.size() == 2);
    store.save({"Carol", 300, 45, 0, 0, 1, 0});
    auto top = store.top(10);
    CHECK(top.size() == 3);
    CHECK(top[0].pseudo == "Carol");
    CHECK(top[0].score == 300);
    std::remove(path.c_str());
}

int main() {
    testRandomizerSevenBag();
    testRotationsAllPieces();
    testWallKickI();
    testLineClearAndScoring();
    testMultiLineClears();
    testBackToBack();
    testPerfectClear();
    testTSpin();
    testLockDelay();
    testHardDropLocksImmediately();
    testPause();
    testGhost();
    testGameOver();
    testHold();
    testScoringFunctions();
    testScoreStore();

    if (failures == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
