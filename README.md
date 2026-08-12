# TETRIS — The Block-Stacking Simulator of Your (Mis)Spent Youth

**100% terminal. 0% graphics card. 100% panic when the I-piece arrives.**

Written in C++17 with ncurses. Pairs beautifully with your 1987 monitor, your
Herman Miller chair, and a lifetime supply of existential dread.

---

## Wait, what is this?

It's Tetris. The one where blocks fall, you stack them, and your amygdala does
the falling. It runs in your terminal, because real computers still have a
terminal, and you should use it.

This build adds the modern conveniences that Pajitnov could only dream of in
1984 — plus a scoring system with enough acronyms (DAS, ARR, T-SPIN, B2B) to
make a defence contractor jealous.

## Building it

You will need:

- A compiler that speaks C++17 (clang++ or g++).
- `ncurses` and its dev headers. If you can't find them, your distro has a
  package for that, and it's exactly the one you were about to `apt install`
  anyway.

```sh
make          # builds ./tetris
make tests    # builds ./tetris_tests, the unit-test armada
./tetris      # crank it up
```

`make clean` erases everything, because that's what we do with feelings.

> **Warning:** building this software may induce nostalgia. Side effects
> include humming the Tetris theme at 3 a.m. and refusing to rotate any
> physical object left unless you have enough room for a wall kick.

## Running it

```
$ ./tetris
```

That's it. There is no save state, no cloud sync, no battle pass. Just you,
a 10×20 well, and seven shapes that have it in for you personally.

## Controls

| Key | Action | 1984 Equivalent |
|-----|--------|-----------------|
| `←` / `j` | Move left | Stare longingly at the joystick |
| `→` / `l` | Move right | Same, the other way |
| `↓` / `s` | Soft drop (tap = 1 row, hold = freefall-ish) | Pressing down harder on the keyboard |
| `↑` / `k` | Rotate clockwise | Twisting the Game Boy angrily |
| `Space` | Hard drop | Letting go of the anxiety |
| `h` / `c` | Hold piece | Putting a block in time-out |
| `n` | Toggle next-piece preview | Blindfold mode, if you insist |
| `p` | Pause | Teleporting to 1999 to answer the landline |
| `q` | Quit to menu | The walk of shame |

The arrow keys always work. The secondary letter keys (`j`/`k`/`l`/`s`/`Space`)
are fully remappable: pick **Settings** in the main menu, press Enter on an
action and then the key you want. The same screen lets you pick the ghost
piece's fill color (left/right arrows or Enter to cycle); its outline stays
black. Your settings are saved to `settings.txt` and restored on the next
launch.

Movement keys feature the famous **DAS/ARR** acceleration — that's
*Delayed Auto Shift* and *Auto Repeat Rate*, two of the finest acronyms ever
imported from the Japanese arcade scene. A single tap moves you exactly one
cell, because we fixed that. You're welcome.

## Features (things that didn't exist on the Game Boy)

- **7-bag randomizer** — every block appears exactly once per bag, because
  even RNG deserves a union.
- **Ghost piece** — a white preview, outlined in black, of where your block
  will land. Fill color is configurable in Settings.
- **Next & Hold** — see the future, keep a spare.
- **Lock delay (500 ms)** — the floor forgives you for 500 ms, up to 15
  resets. In 1989 you'd have been locked out already.
- **Line-clear flash animation** — a glorious 300 ms of strobe, because
  everything was rave in the '90s.
- **T-spin detection** (three-corner rule) — rotate a T into a corner and
  the game screams bonus at you.
- **Combo & Back-to-Back** — consecutive clears stack multipliers like a
  Tetsuya enthusiast stacking tofu.
- **Perfect Clear bonus** — clear the entire board for 3500×level. Never
  happens. We put it in anyway, as a monument to hope.
- **High scores** — persisted to `scores.txt`, because databases are for
  people with time.

### Scoring (the fine print)

| Action | Points (× level) |
|--------|------------------|
| 1 / 2 / 3 / 4 lines | 100 / 300 / 500 / 800 |
| T-spin (no lines) | 400 |
| T-spin 1 / 2 / 3 lines | 800 / 1200 / 1600 |
| Mini T-spin | 100 / 200 |
| Combo | 50 × combo × level |
| Perfect clear | 3500 |
| Back-to-back (Tetris or T-spin) | ×1.5 |

Back-to-back ×1.5 on an 800-point Tetris is 1200. You knew that. We knew you
knew that. This is why we can't have nice things.

## Testing

```sh
make tests && ./tetris_tests
```

A small army of unit tests guards the game logic like mall security: seven-bag
randomizer, all-piece rotations, wall kicks, line clears, T-spins, lock delay,
perfect clears, combo scoring, ghost placement, hold, pause, game over — all
of it. 0 warnings, or the Makefile sulks.

## Architecture (for the person who must know)

```
src/game/        Pure logic. No screen, no keyboard, no emotions.
src/ui/          ncurses front-end. All the input debauchery lives here.
src/storage/     High scores and key bindings in humble text files.
tests/           The aforementioned security guard.
```

The game model is blissfully unaware that a terminal exists. The UI talks to
the model like a nice customer support agent: state queries in, actions out,
no cross-contamination. The model doesn't even know what a keyboard is. It's
the healthiest relationship in this repo.

## License & legacy

This project is a loving homage to a 1984 phenomenon that will outlive us
all. No actual Alexey Pajitnov pixels were harmed. No floppy disks were
required. Your youth is not included — you already spent it.

---

*Stack responsibly. Rotate clockwise. Back-to-back those Tetrises, champ.*
