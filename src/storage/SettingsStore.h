// Persistance des réglages utilisateur (touches + couleur de la pièce
// fantôme) dans un fichier CSV.

#pragma once

#include <string>

namespace tetris {

// Codes de couleur de la pièce fantôme, qui reprennent les valeurs ncurses
// COLOR_* (définis ici pour que la couche de stockage reste indépendante
// de ncurses).
constexpr int kGhostColorBlack = 0;
constexpr int kGhostColorRed = 1;
constexpr int kGhostColorGreen = 2;
constexpr int kGhostColorYellow = 3;
constexpr int kGhostColorBlue = 4;
constexpr int kGhostColorMagenta = 5;
constexpr int kGhostColorCyan = 6;
constexpr int kGhostColorWhite = 7;

// Réglages modifiables par le joueur. Les touches fléchées restent toujours
// disponibles en alternative aux touches secondaires affichées dans l'aide.
// `ghostColor` est un des codes kGhostColor* ci-dessus.
struct Settings {
    int moveLeft = 'j';
    int moveRight = 'l';
    int rotate = 'k';
    int softDrop = 's';
    int hardDrop = ' ';
    int ghostColor = kGhostColorBlack;
};

// Charge et enregistre le fichier de réglages (une paire "nom,valeur" par
// ligne).
class SettingsStore {
public:
    explicit SettingsStore(std::string file = "settings.txt");

    // Lit le fichier ; les entrées manquantes ou invalides gardent leur
    // valeur par défaut.
    Settings load() const;

    // Réécrit les réglages courants dans le fichier.
    void save(const Settings& settings) const;

private:
    std::string file_;
};

} // namespace tetris
