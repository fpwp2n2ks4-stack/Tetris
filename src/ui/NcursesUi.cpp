// Implémentation du front-end ncurses : rendu du jeu et gestion des entrées.

#include "NcursesUi.h"

#include <ncurses.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

namespace tetris {

namespace {

// Identifiants des couples de couleurs ncurses (l'ordre compte : colorId()
// renvoie le couple du type de pièce, soit type + 1).
constexpr int kPairI = 1;
constexpr int kPairO = 2;
constexpr int kPairT = 3;
constexpr int kPairS = 4;
constexpr int kPairZ = 5;
constexpr int kPairJ = 6;
constexpr int kPairL = 7;
constexpr int kPairGhost = 8;
constexpr int kPairFlash = 9;

constexpr long long kDasMs = 150;   // delayed auto shift : délai avant répétition
constexpr long long kArrMs = 50;    // taux de répétition automatique
constexpr long long kReleaseMs = 100; // touche considérée relâchée après ce délai

// Suit si une touche de déplacement est maintenue, en mesurant l'écart entre
// deux événements clavier pour détecter le relâchement (ncurses ne fournit
// pas d'événement de relâchement).
//
// Une répétition n'est armée qu'à la réception d'un *second* événement tant
// que la touche est encore tenue : une simple pression produit un seul
// événement et donc un seul déplacement, tandis qu'un maintien réel (dont la
// répétition du clavier OS génère d'autres événements) active le DAS.
struct DasState {
    bool held = false;
    bool armed_ = false;
    std::chrono::steady_clock::time_point lastEvent{};
    std::chrono::steady_clock::time_point nextRepeat{};

    // Enregistre une pression : arme le DAS à la deuxième pression consécutive.
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

    // Vrai lorsqu'un déplacement de répétition (ARR) est dû maintenant.
    bool repeatDue(std::chrono::steady_clock::time_point now) {
        if (!held || !armed_) return false;
        if (now < nextRepeat) return false;
        nextRepeat += std::chrono::milliseconds(kArrMs);
        return true;
    }

    // Détecte le relâchement : plus d'événement depuis kReleaseMs.
    void releaseCheck(std::chrono::steady_clock::time_point now) {
        if (held && now - lastEvent >= std::chrono::milliseconds(kReleaseMs)) {
            held = false;
            armed_ = false;
        }
    }
};

} // namespace

// Charge les réglages persistés (touches, couleur fantôme) depuis le disque.
NcursesUi::NcursesUi() : settings_(settingsStore_.load()) {}

// Restaure le terminal s'il a été initialisé.
NcursesUi::~NcursesUi() {
    if (initialized_) endwin();
}

// Initialise ncurses : mode cbreak, pas d'écho, clavier étendu et couleurs.
// Les couples de couleurs des pièces et de la fantôme sont déclarés ici.
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
        init_pair(kPairGhost, COLOR_BLACK,
                  static_cast<short>(settings_.ghostColor & 0x7));
        init_pair(kPairFlash, COLOR_WHITE, COLOR_BLACK);
    }
    initialized_ = true;
    return true;
}

// Ferme l'écran ncurses (aucun effet si déjà fermé).
void NcursesUi::shutdown() {
    if (initialized_) {
        endwin();
        initialized_ = false;
    }
}

// Identifiant du couple de couleurs d'un type de pièce (voir les constantes
// kPair* ci-dessus).
int NcursesUi::colorId(PieceType type) {
    return static_cast<int>(type) + 1;
}

// Formate un nombre de secondes en "HH:MM:SS".
std::string NcursesUi::formatTime(int seconds) {
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    const int s = seconds % 60;
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02d:%02d:%02d", h, m, s);
    return buf;
}

// Traduit un code clavier (caractère ou KEY_* de ncurses) en libellé
// lisible pour l'afficher dans les menus et l'aide.
std::string NcursesUi::keyName(int key) {
    switch (key) {
        case ' ': return "Space";
        case KEY_LEFT: return "Left";
        case KEY_RIGHT: return "Right";
        case KEY_UP: return "Up";
        case KEY_DOWN: return "Down";
        case KEY_ENTER: return "Enter";
        case '\n': return "Enter";
        case '\r': return "Enter";
        case '\t': return "Tab";
        case 27: return "Esc";
        case KEY_BACKSPACE: return "Backspace";
        default: break;
    }
    if (key >= 32 && key <= 126) {
        return std::string(1, static_cast<char>(key));
    }
    return std::to_string(key);
}

// Vrai si le code clavier `ch` correspond à la touche configurée `bound` ;
// pour une lettre, la majuscule et la minuscule sont équivalentes.
bool NcursesUi::keyMatches(int ch, int bound) {
    if (ch == bound) return true;
    const unsigned char b = static_cast<unsigned char>(bound);
    if (std::isalpha(b)) {
        return ch == std::toupper(b) || ch == std::tolower(b);
    }
    return false;
}

// Traduit un code couleur (0..7, valeurs ncurses COLOR_*) en nom lisible.
std::string NcursesUi::colorName(int color) {
    static const std::array<const char*, 8> names = {
        "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
    };
    return names[static_cast<std::size_t>(color & 0x7)];
}

// Dessine une cellule du plateau selon son identifiant : vide (". "),
// pièce fantôme (colorée et atténuée), ligne en cours de suppression
// (clignotement) ou pièce normale.
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

// Dessine une rangée (sur 4) de la mini-grille d'une pièce du panneau
// latéral (hold / next).
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

// Dessine le panneau latéral (HOLD et NEXT) aligné sur la rangée de plateau
// `row` : en-têtes, mini-grilles des pièces ou espaces vides.
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

// Dessine une frame complète : plateau (avec pièce fantôme, pièce courante
// et lignes en cours de suppression), panneau latéral, statistiques, dernier
// événement et barre d'aide des touches configurées.
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

    const auto& lc = game.lineClears();
    printw("\n1L: %d    2L: %d    3L: %d    Tetris: %d",
           lc[0], lc[1], lc[2], lc[3]);
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
    printw("Arrows: move/rotate/soft drop   %s: left   %s: right   %s: rotate   "
           "%s: soft drop   %s: hard drop   h: hold   n: next   p: pause   q: quit\n",
           keyName(settings_.moveLeft).c_str(), keyName(settings_.moveRight).c_str(),
           keyName(settings_.rotate).c_str(), keyName(settings_.softDrop).c_str(),
           keyName(settings_.hardDrop).c_str());
    attroff(A_DIM);
    refresh();
}

// Boucle de jeu principale : rend la frame, lit les entrées (avec DAS/ARR
// pour les déplacements), met à jour le modèle et s'arrête à la fin de la
// partie ou sur 'q'.
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
            if (ch == 'q' || ch == 'Q') {
                running = false;
            } else if (ch == 'p' || ch == 'P') {
                game.togglePause();
            } else if (ch == KEY_LEFT || keyMatches(ch, settings_.moveLeft)) {
                if (!left.held) game.moveLeft();
                left.press(now);
            } else if (ch == KEY_RIGHT || keyMatches(ch, settings_.moveRight)) {
                if (!right.held) game.moveRight();
                right.press(now);
            } else if (ch == KEY_DOWN || keyMatches(ch, settings_.softDrop)) {
                if (!down.held) game.softDrop();
                down.press(now);
            } else if (ch == KEY_UP || keyMatches(ch, settings_.rotate)) {
                game.rotateCW();
            } else if (keyMatches(ch, settings_.hardDrop)) {
                game.hardDrop();
            } else if (ch == 'h' || ch == 'H' || ch == 'c' || ch == 'C') {
                game.holdPiece();
            } else if (ch == 'n' || ch == 'N') {
                showNextPiece_ = !showNextPiece_;
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

// Écran de fin de partie : statistiques puis saisie du pseudo (libre, écho
// actif). Un pseudo vide devient "Anonymous".
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

// Affiche le tableau des meilleurs scores (10 premières entrées) et attend
// une touche pour revenir au menu.
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

// Écran de configuration : remappe les cinq actions de jeu et change la
// couleur de la pièce fantôme. Vérifie les conflits de touches, permet de
// réinitialiser (r) et enregistre les changements dans settings.txt.
void NcursesUi::showKeyBindings() {
    struct Action {
        const char* name;
        int Settings::* field;
        bool isColor;
    };
    const Action actions[6] = {
        {"Move Left", &Settings::moveLeft, false},
        {"Move Right", &Settings::moveRight, false},
        {"Rotate", &Settings::rotate, false},
        {"Soft Drop", &Settings::softDrop, false},
        {"Hard Drop", &Settings::hardDrop, false},
        {"Ghost Color", &Settings::ghostColor, true},
    };
    constexpr int kCount = 6;
    int selection = 0;
    bool changed = false;
    std::string message;

    while (true) {
        clear();
        attron(A_BOLD);
        printw("SETTINGS\n");
        attroff(A_BOLD);
        printw("\n");
        for (int i = 0; i < kCount; ++i) {
            const std::string value = actions[i].isColor
                                          ? colorName(settings_.*(actions[i].field))
                                          : keyName(settings_.*(actions[i].field));
            if (i == selection) attron(A_REVERSE);
            printw("  %-12s : %s\n", actions[i].name, value.c_str());
            if (i == selection) attroff(A_REVERSE);
        }
        printw("\n%s", message.c_str());
        printw("Arrows move (or change color), Enter to change, "
               "r to reset, q to return\n");
        refresh();

        timeout(-1);
        const int ch = getch();
        message.clear();
        const bool onColor = actions[selection].isColor;
        if (ch == KEY_UP || ch == 'k' || ch == 'K') {
            selection = (selection - 1 + kCount) % kCount;
        } else if (ch == KEY_DOWN || ch == 'j' || ch == 'J') {
            selection = (selection + 1) % kCount;
        } else if (onColor && (ch == KEY_LEFT || ch == KEY_RIGHT ||
                               ch == '\n' || ch == '\r' || ch == KEY_ENTER)) {
            int& color = settings_.*(actions[selection].field);
            if (ch == KEY_LEFT) {
                color = (color + 7) & 0x7;
            } else {
                color = (color + 1) & 0x7;
            }
            changed = true;
            message = "Ghost color set to " + colorName(color) + ".";
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            clear();
            printw("Press a new key for '%s' (Esc to cancel)\n",
                   actions[selection].name);
            refresh();
            const int newKey = getch();
            if (newKey == 27 || newKey == ERR || newKey == -1) {
                message = "Key change cancelled.";
                continue;
            }
            int conflicting = -1;
            for (int i = 0; i < kCount; ++i) {
                if (i != selection && keyMatches(newKey, settings_.*(actions[i].field))) {
                    conflicting = i;
                    break;
                }
            }
            if (conflicting != -1) {
                message = std::string("Key already used for '") +
                          actions[conflicting].name + "'.";
                continue;
            }
            if (keyMatches(newKey, 'q') || keyMatches(newKey, 'p') ||
                keyMatches(newKey, 'h') || keyMatches(newKey, 'c') ||
                keyMatches(newKey, 'n')) {
                message = "Key conflicts with a fixed action (q/p/h/c/n).";
                continue;
            }
            settings_.*(actions[selection].field) = newKey;
            changed = true;
            message = std::string("'") + keyName(newKey) + "' bound to " +
                      actions[selection].name + ".";
        } else if (ch == 'r' || ch == 'R') {
            settings_ = Settings{};
            changed = true;
            message = "Settings reset to defaults.";
        } else if (ch == 'q' || ch == 'Q') {
            break;
        }
    }

    if (changed) {
        settingsStore_.save(settings_);
        init_pair(kPairGhost, COLOR_BLACK,
                  static_cast<short>(settings_.ghostColor & 0x7));
    }
}

// Menu principal : navigation aux flèches (ou j/k), sélection à Entrée.
// Les entrées "Next Piece" et "Settings" basculent l'état / ouvrent l'écran
// de réglages ; "Quit" renvoie 2 pour terminer le programme.
int NcursesUi::showMainMenu() {
    int selection = 0;

    while (true) {
        const std::vector<std::string> items = {
            "Play",
            "High Scores",
            showNextPiece_ ? "Next Piece: On" : "Next Piece: Off",
            "Settings",
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
                if (selection == 3) { // key bindings
                    showKeyBindings();
                    break;
                }
                return selection == 4 ? 2 : selection;
            case 'q':
            case 'Q':
                return 2;
            default:
                break;
        }
    }
}

} // namespace tetris
