// TETRIS — point d'entrée du programme.
//
// Relie le modèle de jeu pur à l'interface ncurses et aux deux couches de
// stockage, puis exécute la boucle principale : menu -> partie -> écran de
// fin/pseudo -> meilleurs scores, jusqu'à ce que le joueur quitte.

#include "game/Game.h"
#include "storage/ScoreStore.h"
#include "ui/NcursesUi.h"

#include <string>

// Lance toute l'application. Renvoie 0 en cas de sortie propre.
int main() {
    tetris::ScoreStore store;
    tetris::NcursesUi ui;
    if (!ui.init()) return 1;

    while (true) {
        const int choice = ui.showMainMenu();
        if (choice == 2) break;
        if (choice == 1) {
            ui.showHighScores(store);
            continue;
        }

        tetris::Game game;
        game.start();
        const bool ended = ui.playGame(game);
        if (!ended) continue;

        std::string pseudo = ui.promptPseudo(game);
        const auto& lc = game.lineClears();
        store.save({pseudo, game.score(), game.durationSeconds(),
                    lc[0], lc[1], lc[2], lc[3]});
    }

    ui.shutdown();
    return 0;
}
