# Hagnus Miemann Chess Engine

Alex, Josiah, and Justin have created the greatest chess engine ever built for a UWaterloo second-year CS course.
Here it is.

We are lazy and don't want to describe what it does, so here is a copy-paste of the manual:

### Manual
```
 ◌ ╭─────────────────────────────────────╮
 ◌ │ HAGNUS MIEMAN CHESS ENGINE - Manual │
 ◌ ╰─────────────────────────────────────╯
 ◌ ╭──╴
 ◌ │ game [white] [black]
 ◌ │         Options are `player` and `computer[1-4]`.
 ◌ │ man
 ◌ │         Opens this manual. `help` also does.
 ◌ │ move
 ◌ │         Tells the computer to play its move.
 ◌ │ move [from] [to] [promotion]
 ◌ │         Play a move, for example: `move g2 g1 R`.
 ◌ │ perft [0-50]
 ◌ │         Run a PERFT test on the current board.
 ◌ │ resign
 ◌ │         Resigns the current game.
 ◌ │ score
 ◌ │         Display the current scores of White and Black players.
 ◌ │ settings
 ◌ │         Display the current settings.
 ◌ │ setup [FEN]
 ◌ │         Initialize a game with a well-formed FEN.
 ◌ │ setup
 ◌ │         Enters setup mode, with these methods:
 ◌ │         + [piece] [at]
 ◌ │                 Place `piece` at square `at`, on top of whatever is there.
 ◌ │         - [at]
 ◌ │                 Remove any piece at square `at`.
 ◌ │         = [colour]
 ◌ │                 Make it `colour`'s turn to play. Can be `white` or `black`.
 ◌ │         done
 ◌ │                 Exit setup mode, if restrictions are met.
 ◌ │ toggle [0-3]
 ◌ │         Toggle the numbered setting.
 ◌ │ undo
 ◌ │         Undoes the previous move in the current game.
 ◌ ╰──╴
```
