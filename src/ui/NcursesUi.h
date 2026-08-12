// Interface ncurses : affiche le modèle de jeu et traduit les entrées
// clavier en actions sur Game. Gère aussi les menus (principal, réglages,
// meilleurs scores).

#pragma once

#include "../game/Game.h"
#include "../storage/ScoreStore.h"
#include "../storage/SettingsStore.h"

#include <optional>
#include <string>

namespace tetris {

// Contenu de rendu d'une cellule du plateau : couple de couleurs de fond et
// glyphes de contour éventuels (caractères ACS).
struct CellRender {
    int pair = 0;     // identifiant du couple de couleurs ; 0 = cellule vide
    int g0 = 0;       // glyphe de contour (moitié gauche) ; 0 = remplissage
    int g1 = 0;       // glyphe de contour (moitié droite) ; 0 = remplissage
    bool dim = false; // applique A_DIM au remplissage (pièce fantôme)
};

// Front-end ncurses : rend l'état du modèle et mappe le clavier vers les
// actions du jeu. Ne contient aucune règle de jeu.
class NcursesUi {
public:
    NcursesUi();
    ~NcursesUi();

    NcursesUi(const NcursesUi&) = delete;
    NcursesUi& operator=(const NcursesUi&) = delete;

    // Initialise l'écran ncurses (couleurs comprises). False si échec.
    bool init();

    // Restaure le terminal (l'appel à endwin est idempotent ici).
    void shutdown();

    // Affiche le menu principal. Renvoie 0 = jouer, 1 = scores, 2 = quitter.
    int showMainMenu();

    // Boucle de jeu : rend, lit le clavier, met à jour le modèle. Renvoie
    // true si la partie s'est terminée, false si le joueur a quitté en cours
    // de partie (aucun score n'est alors enregistré).
    bool playGame(Game& game);

    // Écran de fin de partie ; renvoie le pseudo saisi par le joueur.
    std::string promptPseudo(const Game& game);

    // Affiche le tableau des meilleurs scores.
    void showHighScores(const ScoreStore& store);

    // Écran de configuration des touches et de la couleur fantôme.
    void showKeyBindings();

private:
    // Identifiant de couple de couleurs ncurses d'un type de pièce.
    static int colorId(PieceType type);

    // Formate une durée en secondes au format HH:MM:SS.
    static std::string formatTime(int seconds);

    // Nom lisible d'un code clavier ncurses (ex. "Space", "Left").
    static std::string keyName(int key);

    // Nom lisible d'un code couleur (ex. "Red").
    static std::string colorName(int color);

    // Vrai si `ch` correspond à la touche `bound` (en gérant majuscule/
    // minuscule pour les lettres).
    static bool keyMatches(int ch, int bound);

    // Dessine une frame complète du jeu sur l'écran.
    void renderGame(const Game& game) const;

    // Dessine une cellule du plateau d'après son contenu de rendu.
    void printCell(const CellRender& cell) const;

    // Dessine une ligne de la mini-grille (hold/next) d'une pièce.
    void printMiniLine(const std::optional<PieceType>& piece, int row) const;

    // Dessine le panneau latéral (hold + next) pour une rangée donnée.
    void printSidePanel(const Game& game, int row) const;

    bool initialized_ = false;
    bool showNextPiece_ = true;
    SettingsStore settingsStore_;
    Settings settings_;
};

} // namespace tetris
