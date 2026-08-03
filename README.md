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

Movement keys feature the famous **DAS/ARR** acceleration — that's
*Delayed Auto Shift* and *Auto Repeat Rate*, two of the finest acronyms ever
imported from the Japanese arcade scene. A single tap moves you exactly one
cell, because we fixed that. You're welcome.

## Features (things that didn't exist on the Game Boy)

- **7-bag randomizer** — every block appears exactly once per bag, because
  even RNG deserves a union.
- **Ghost piece** — a see-through preview of where your block will land.
  Useful for people who can't do subtraction.
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

## License & legacy
MIT Licence
This project is a loving homage to a 1984 phenomenon that will outlive us
all. No actual Alexey Pajitnov pixels were harmed. No floppy disks were
required. Your youth is not included — you already spent it.
