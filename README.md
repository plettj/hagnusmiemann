# Hagnus Miemann Chess Engine

Alex, Josiah, and Justin have created the greatest chess engine ever built for a UWaterloo second-year course.

We are lazy and don't want to describe what it does, so here is a copy-paste of the manual:

### Manual
```
 ◌ ╭──────────────────────────────────────╮
 ◌ │ HAGNUS MIEMANN CHESS ENGINE - Manual │
 ◌ ╰──────────────────────────────────────╯
 ◌ ╭─────╴
 ◌ ╞╴ ./chess
 ◌ │         Captures programmers who have no short-term memory.
 ◌ ╞╴ close
 ◌ │         Force-quits the current game, without awarding points.
 ◌ ╞╴ exit
 ◌ │         Immediately terminates the program.
 ◌ ╞╴ game [white] [black]
 ◌ │         Starts a new game. Options are `player` and `computer[1-6]`.
 ◌ ╞╴ graphics [size]
 ◌ │         Opens a graphical display of width `size`.
 ◌ ╞╴ graphics
 ◌ │         Closes the graphical observer.
 ◌ ╞╴ help
 ◌ │         Opens this manual.
 ◌ ╞╴ make
 ◌ │         Captures programmers who forgot to CTRL+C.
 ◌ ╞╴ move
 ◌ │         Tells the computer to compute and play its move.
 ◌ ╞╴ move [from] [to] [promotion?]
 ◌ │         Plays a move. For example: `move e1 g1` or `move g2 g1 R`.
 ◌ ╞╴ perft [0-15]
 ◌ │         Runs a PERFT test on the current board.
 ◌ ╞╴ print
 ◌ │         Displays the current game.
 ◌ ╞╴ quit
 ◌ │         Submits EOF; displays the final scores and exits the program.
 ◌ ╞╴ resign
 ◌ │         Resigns the current game.
 ◌ ╞╴ scores
 ◌ │         Displays the current scores of White and Black players.
 ◌ ╞╴ secret
 ◌ │         Literally just doesn't do anything.
 ◌ ╞╴ settings
 ◌ │         Displays the current settings.
 ◌ ╞╴ setup [FEN]
 ◌ │         Initializes a game with a well-formed FEN.
 ◌ ╞╴ setup
 ◌ ╰──╮      Enters setup mode, which has the following methods:
 ◌    ╞╴ + [piece] [square]
 ◌    │          Places `piece` at square `square`, on top of whatever is there.
 ◌    ╞╴ - [square]
 ◌    │          Removes any piece at square `square`.
 ◌    ╞╴ = [colour]
 ◌    │          Makes it `colour`'s turn to play.
 ◌    ╞╴ cancel
 ◌    │          Leaves setup mode and resets the board.
 ◌    ╞╴ castles
 ◌    │          Displays the current castling rights.
 ◌    ╞╴ done
 ◌    │          Completes setup mode, if restrictions are met.
 ◌    ╞╴ help
 ◌    │          Prints the setup mode manual.
 ◌    ╞╴ passant [square]
 ◌    │          Sets the en passant square.
 ◌    ╞╴ print
 ◌    │          Displays the current board.
 ◌    ╞╴ toggle [right]
 ◌ ╭──╯          Toggles the specified castling right.
 ◌ ╞╴ toggle [0-3]
 ◌ │         Toggles the numbered setting.
 ◌ ╞╴ undo
 ◌ │         Undoes the previous move in the current game.
 ◌ ╰─────╴
```
