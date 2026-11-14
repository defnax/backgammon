# Backgammon

A RetroShare plugin to play **Backgammon** and **Long Nardy** with any
RetroShare identity. 

<img width="990" height="664" alt="image" src="https://github.com/user-attachments/assets/53bdcbd9-b9fc-4e41-bcf3-6c7221804b0b" />


## Build

Put the folder in `RetroShare/plugins/` (next to RetroChess) and build it like
the other plugins, with Qt 5.15 or Qt 6:

```
cd plugins/Backgammon
qmake Backgammon.pro
make            # MinGW: mingw32-make
```

Or with CMake, from the RetroShare super-project or standalone (on Windows
standalone, point `-DRS_BUILD_DIR` at a RetroShare build):

```
cmake -S plugins/Backgammon -B build-backgammon
cmake --build build-backgammon
```

With qmake the plugin ends up in `lib/` (`Backgammon.dll` on Windows,
`libBackgammon.so` on Linux). Copy it to RetroShare's plugin folder and
enable it in *Settings → Plugins*. A C++17 compiler is enough.

The rules core has its own tests, built without Qt (or with CMake:
`-DBACKGAMMON_BUILD_TESTS=ON`, then `ctest`):

```
g++ -std=c++17 -I core tests/core-test.cpp core/BgRules.cpp core/BgMatch.cpp -o core-test
./core-test
```

## Playing

Open the **Backgammon** page, choose the identity to play with, the game
(Backgammon or Long Nardy), the number of points to win and an opponent, then
click **Invite to play**. The opponent gets a game window with *Accept* and
*Decline*. Players roll, move checkers by dragging them (or clicking a checker,
then its destination; a checker with only one possible move goes there on a
click), and click *End turn*. A roll that cannot be played passes by itself.
*Resign* (or closing the window) gives the match to the opponent.

The opponent comes from one of two tabs: **My players**, a list of identities
kept between sessions (everyone you have played with is added, and others can
be added from the other tab with a right-click), and **All identities**.
Double-click a player to invite them; right-click to invite or remove.

A new invitation shows in **Your games** (in bold, with *Accept* and *Decline*
buttons and a right-click menu), as a toaster with an *Accept* button, and as a
counter on the Backgammon toolbar button. *Preferences → Backgammon* chooses
whether the game window opens by itself and whether the toaster shows (the
toaster can also be switched off in *Preferences → Notify*).

*Only accept invitations from my contacts* (in *Preferences → Backgammon*) ignores invitations from
identities that are not in your contacts. Banned identities are always
ignored.

*Preferences → Backgammon* also chooses the board colours: Light wood
(default), RetroShare blue, Walnut, Red and black on sand, Red and black on
brown, Green felt and Night. Themes live in `gui/BoardTheme.cpp`; open games
change colour right away.

The game plays sounds (from `sounds/`) for invitations, dice, checker moves,
the opponent's turns, resignations and results. *Preferences → Backgammon*
turns them on or off one by one and sets the volume; double-click an entry
to hear it. Nothing plays while RetroShare's sounds are muted.

## How it works

* **Transport.** Every match uses a secured GXS tunnel between the two
  identities (client service id `0x42474d4e`, "BGMN"). The players do not
  need to be friends. RetroShare keeps one tunnel per pair of identities and
  shares it between all services that use GXS tunnels, so it can go down or be
  closed by another service while a match is running. That does not end the
  match: the plugin asks for the tunnel again every 10 seconds for up to
  3 minutes; once it is back, both sides exchange their position (`P`/`Q`) and
  a turn that was lost meanwhile is sent again. Only `R` (resign) or `X`
  (quit) means the opponent left.
* **Packets.** `"RBG2" | match token (16 bytes) | op | JSON`, with ops
  `I` invite, `O` invitation received (lets the inviter know the opponent is
  there and has the plugin), `A` accept, `S` start, `D` dice rolled, `T` turn (moves as text,
  `"24/18 13/11"`, `"bar/20"`, `"6/off"`), `C` double,
  `K` take, `N` drop, `P`/`Q` position after a reconnection, `R` resign and
  `X` leave. See `services/BackgammonService.h`.
* **Doubling cube.** In matches of 2 points or more, the player on roll may
  click *Double* before rolling if the cube is in the middle or theirs. The
  opponent takes (the cube doubles and becomes theirs) or drops (losing the
  game at the current value). Game points are multiplied by the cube. The
  Crawford rule applies: no doubling in the game right after a player first
  gets one point away from winning the match.
* **Fair dice.** The inviter puts a commitment `SHA-256(A)` of a random nonce
  `A` in the invitation, the opponent answers with its own nonce `B`, then the
  inviter reveals `A`. Both peers seed the dice from `A` and `B`; the seed also
  decides who plays white. Neither player can choose the rolls, and the dice
  never travel over the network: each peer computes them. `bg::Dice` maps
  `std::mt19937` output to 1–6 itself, because `std::uniform_int_distribution`
  gives different results with different standard libraries.
* **Move checking.** A turn received from the opponent is only played if it is
  one of the legal turns the local engine computed for that roll. Anything
  else stops the match.

## Rules

`core/` holds the rules, in plain C++ without Qt:

* **Backgammon:** hitting and entering from the bar, blocked points, using as
  many dice as possible and the larger die when only one can be played,
  bearing off (a larger number only from the highest point), gammon and
  backgammon, the opening roll (each side throws one die, the higher one
  starts with both numbers), doubling cube and Crawford rule.
* **Long Nardy:** both sides move the same way round from their head, a single
  checker blocks a point, no hitting, one checker from the head per turn
  (two with 6-6, 4-4 or 3-3 on the first turn), no six points in a row unless
  an opposing checker has passed them, mars counts double. No cube.

## Layout

| Folder      | Content |
|-------------|---------|
| `core/`     | Rules: `BgRules` (positions, legal turns, dice, game value) and `BgMatch` (games, cube, Crawford rule, score) |
| `gui/`      | Main page, game window, settings page, board widget, themes and sounds. All windows are Designer `.ui` files. |
| `services/` | `BackgammonService`: GXS tunnel client and match protocol |


