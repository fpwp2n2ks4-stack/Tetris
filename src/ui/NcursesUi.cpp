#include "NcursesUi.h"

#include <ncurses.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace tetris {

namespace {

constexpr int kPairI = 1;
constexpr int kPairO = 2;
constexpr int kPairT = 3;
constexpr int kPairS = 4;
constexpr int kPairZ = 5;
constexpr int kPairJ = 6;
constexpr int kPairL = 7;
constexpr int kPairGhost = 8;
constexpr int kPairFlash = 9;

constexpr long long kDasMs = 150;   // delayed auto shift: held-key delay
constexpr long long kArrMs = 50;    // auto repeat rate
constexpr long long kReleaseMs = 100; // key considered released after this gap

// Tracks whether a movement key is being held, using the gap between key
// events to detect release (ncurses does not deliver key-up events).
//
// A repeat is only armed once a *second* event arrives while the key is
// still held: a single tap produces one event and therefore exactly one
// move, while a genuine hold (whose OS-level key repeat produces further
// events) enables delayed auto shift.
struct DasState {
    bool held = false;
    bool armed_ = false;
    std::chrono::steady_clock::time_point lastEvent{};
    std::chrono::steady_clock::time_point nextRepeat{};

    void press(std::chrono::steady_clock::time_point now) {
        if (!held) {
            held = true;
            armed_ = false;
        } else if (!armed_) {
            armed_ = true;
            nextRepeat = now + std::chrono::milliseconds(kDasMs);
        }
        lastEvent = now;
    }
    bool repeatDue(std::chrono::steady_clock::time_point now) {
        if (!held || !armed_) return false;
        if (now < nextRepeat) return false;
        nextRepeat += std::chrono::milliseconds(kArrMs);
        return true;
    }
    void releaseCheck(std::chrono::steady_clock::time_point now) {
        if (held && now - lastEvent >= std::chrono::milliseconds(kReleaseMs)) {
            held = false;
            armed_ = false;
        }
    }
};

} // namespace

NcursesUi::~NcursesUi() {
    if (initialized_) endwin();
}

bool NcursesUi::init() {
    if (!initscr()) return false;
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        init_pair(kPairI, COLOR_WHITE, COLOR_CYAN);
        init_pair(kPairO, COLOR_WHITE, COLOR_YELLOW);
        init_pair(kPairT, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(kPairS, COLOR_WHITE, COLOR_GREEN);
        init_pair(kPairZ, COLOR_WHITE, COLOR_RED);
        init_pair(kPairJ, COLOR_WHITE, COLOR_BLUE);
        init_pair(kPairL, COLOR_BLACK, COLOR_WHITE);
        init_pair(kPairGhost, COLOR_BLACK, COLOR_WHITE);
        init_pair(kPairFlash, COLOR_WHITE, COLOR_BLACK);
    }
    initialized_ = true;
    return true;
}

void NcursesUi::shutdown() {
    if (initialized_) {
        endwin();
        initialized_ = false;
    }
}

int NcursesUi::colorId(PieceType type) {
    return static_cast<int>(type) + 1;
}

std::string NcursesUi::formatTime(int seconds) {
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    const int s = seconds % 60;
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d:%02d:%02d", h, m, s);
    return buf;
}

void NcursesUi::printCell(int id) const {
    if (id == 0) {
        printw(". ");
    } else if (id == kPairGhost) {
        attron(A_DIM | COLOR_PAIR(kPairGhost));
        printw("  ");
        attroff(A_DIM | COLOR_PAIR(kPairGhost));
    } else if (id == kPairFlash) {
        attron(A_REVERSE | COLOR_PAIR(kPairFlash));
        printw("  ");
        attroff(A_REVERSE | COLOR_PAIR(kPairFlash));
    } else {
        attron(COLOR_PAIR(id));
        printw("  ");
        attroff(COLOR_PAIR(id));
    }
}

void NcursesUi::printMiniLine(const std::optional<PieceType>& piece,
                              int row) const {
    static const std::vector<std::vector<int>> kEmptyShape;
    const auto& shape = piece ? baseShape(*piece) : kEmptyShape;
    const int n = piece ? static_cast<int>(shape.size()) : 0;
    for (int c = 0; c < 4; ++c) {
        const bool filled =
            piece && row < n && c < n &&
            shape[static_cast<std::size_t>(row)][static_cast<std::size_t>(c)];
        if (filled) {
            attron(COLOR_PAIR(colorId(*piece)));
            printw("  ");
            attroff(COLOR_PAIR(colorId(*piece)));
        } else {
            printw("  ");
        }
    }
}

void NcursesUi::printSidePanel(const Game& game, int row) const {
    if (row == 0) {
        printw("%-8s", "HOLD");
        printw("  ");
        if (showNextPiece_) {
            printw("%-8s", "NEXT");
        } else {
            printw("        ");
        }
        return;
    }
    if (row >= 1 && row <= 4) {
        printMiniLine(game.hold(), row - 1);
        printw("  ");
        if (showNextPiece_) {
            printMiniLine(game.next(), row - 1);
        } else {
            printw("        ");
        }
        return;
    }
    printw("        ");
    printw("  ");
    printw("        ");
}

void NcursesUi::renderGame(const Game& game) const {
    clear();

    std::array<std::array<int, kCols>, kRows> display{};
    const auto& board = game.board();
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] =
                board[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
        }
    }

    if (game.state() == GameState::Playing) {
        if (auto g = game.ghost()) {
            for (auto [dr, dc] : occupiedCells(g->type, g->rotation)) {
                const int r = g->row + dr;
                const int c = g->col + dc;
                if (r >= 0 && r < kRows && c >= 0 && c < kCols &&
                    display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] == 0) {
                    display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = kPairGhost;
                }
            }
        }
        if (auto cur = game.current()) {
            for (auto [dr, dc] : occupiedCells(cur->type, cur->rotation)) {
                const int r = cur->row + dr;
                const int c = cur->col + dc;
                if (r >= 0 && r < kRows && c >= 0 && c < kCols) {
                    display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = colorId(cur->type);
                }
            }
        }
    }

    if (game.phase() == Phase::Clearing) {
        for (int r : game.clearingRows()) {
            for (int c = 0; c < kCols; ++c) {
                display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = kPairFlash;
            }
        }
    }

    for (int r = 0; r < kRows; ++r) {
        printSidePanel(game, r);
        printw("  ");
        for (int c = 0; c < kCols; ++c) {
            printCell(display[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]);
        }
        printw("\n");
    }

    printw("\nScore: %d    Lines: %d    Level: %d",
           game.score(), game.lines(), game.level());
    if (game.combo() > 1) {
        printw("    Combo: x%d", game.combo());
    }
    if (game.state() == GameState::Paused) {
        printw("    [PAUSED]");
    }
    printw("\n");
    if (!game.lastEvent().empty()) {
        attron(A_BOLD);
        printw("%s\n", game.lastEvent().c_str());
        attroff(A_BOLD);
    }
    attron(A_DIM);
    printw("Arrows or j/k/l/s move-rotate  Space: hard drop  "
           "h: hold  n: next piece  p: pause  q: quit\n");
    attroff(A_DIM);
    refresh();
}

bool NcursesUi::playGame(Game& game) {
    using Clock = std::chrono::steady_clock;

    DasState left, right, down;
    bool running = true;
    bool ended = false;

    renderGame(game);

    while (running) {
        const long long wait = game.nextEventInMs();
        timeout(static_cast<int>(std::clamp(wait, 1LL, 50LL)));

        const int ch = getch();
        auto now = Clock::now();
        if (ch != ERR && ch != -1) {
            switch (ch) {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case 'p':
                case 'P':
                    game.togglePause();
                    break;
                case KEY_LEFT:
                case 'j':
                case 'J':
                    if (!left.held) game.moveLeft();
                    left.press(now);
                    break;
                case KEY_RIGHT:
                case 'l':
                case 'L':
                    if (!right.held) game.moveRight();
                    right.press(now);
                    break;
                case KEY_DOWN:
                case 's':
                case 'S':
                    if (!down.held) game.softDrop();
                    down.press(now);
                    break;
                case KEY_UP:
                case 'k':
                case 'K':
                    game.rotateCW();
                    break;
                case ' ':
                    game.hardDrop();
                    break;
                case 'h':
                case 'H':
                case 'c':
                case 'C':
                    game.holdPiece();
                    break;
                case 'n':
                case 'N':
                    showNextPiece_ = !showNextPiece_;
                    break;
                default:
                    break;
            }
        }

        now = Clock::now();
        if (left.repeatDue(now)) game.moveLeft();
        if (right.repeatDue(now)) game.moveRight();
        if (down.repeatDue(now)) game.softDrop();
        left.releaseCheck(now);
        right.releaseCheck(now);
        down.releaseCheck(now);

        game.update(now);

        if (game.state() == GameState::GameOver) {
            ended = true;
            running = false;
        }
        renderGame(game);
    }
    return ended;
}

std::string NcursesUi::promptPseudo(const Game& game) {
    clear();
    printw("--- GAME OVER ---\n\n");
    printw("Score: %d\n", game.score());
    printw("Time: %s\n", formatTime(game.durationSeconds()).c_str());
    printw("Level reached: %d\n", game.level());
    const auto& lc = game.lineClears();
    printw("Lines cleared:  Single: %d  Double: %d  Triple: %d  Tetris: %d\n",
           lc[0], lc[1], lc[2], lc[3]);
    printw("\nEnter your name: ");
    refresh();

    timeout(-1);
    echo();
    char name[40] = {0};
    getstr(name);
    noecho();

    std::string pseudo(name);
    if (pseudo.empty()) pseudo = "Anonymous";
    return pseudo;
}

void NcursesUi::showHighScores(const ScoreStore& store) {
    clear();
    printw("--- HIGH SCORES ---\n\n");

    auto scores = store.top(10);
    if (scores.empty()) {
        printw("No scores recorded yet.\n");
    } else {
        printw("%-3s %-15s %-8s %-12s %-6s %-6s %-6s %-6s\n",
               "Rk", "Pseudo", "Score", "Time", "1L", "2L", "3L", "4L");
        printw("--------------------------------------------------------------\n");
        int rank = 1;
        for (const auto& e : scores) {
            printw("%-3d %-15s %-8d %-12s %-6d %-6d %-6d %-6d\n",
                   rank++, e.pseudo.c_str(), e.score,
                   formatTime(e.timeSeconds).c_str(),
                   e.lines1, e.lines2, e.lines3, e.lines4);
        }
    }

    printw("\nPress any key to return...");
    refresh();
    timeout(-1);
    getch();
}

int NcursesUi::showMainMenu() {
    int selection = 0;

    while (true) {
        const std::vector<std::string> items = {
            "Play",
            "High Scores",
            showNextPiece_ ? "Next Piece: On" : "Next Piece: Off",
            "Quit",
        };
        const int count = static_cast<int>(items.size());

        clear();
        attron(A_BOLD);
        printw("TETRIS\n");
        attroff(A_BOLD);
        printw("\n");
        for (int i = 0; i < count; ++i) {
            if (i == selection) attron(A_REVERSE);
            printw("  %s\n", items[static_cast<std::size_t>(i)].c_str());
            if (i == selection) attroff(A_REVERSE);
        }
        printw("\nArrows and Enter to select (q to quit)\n");
        refresh();

        timeout(-1);
        const int ch = getch();
        switch (ch) {
            case KEY_UP:
            case 'k':
                selection = (selection - 1 + count) % count;
                break;
            case KEY_DOWN:
            case 'j':
                selection = (selection + 1) % count;
                break;
            case '\n':
            case '\r':
            case KEY_ENTER:
                if (selection == 2) { // toggle next-piece preview
                    showNextPiece_ = !showNextPiece_;
                    break;
                }
                return selection == 3 ? 2 : selection;
            case 'q':
            case 'Q':
                return 2;
            default:
                break;
        }
    }
}

} // namespace tetris
