#include "board.h"
#include "move.h"
#include "io.h"
#include <iostream>

int main() {
    //Board board = Board::createBoardFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    //board.perftTest(5);

    IO io{std::cin};
    io.makeTextOutput(std::cout);

    // This call starts the program running!
    // COMMANDS WE SUPPORT:
    // 1. game [white] [black]      Can be player or computer[1-4]
    // 2. resign                    Concedes the game to the opponent
    // 3. move                      Makes the computer play a move
    // 4. move [sq1] [sq2] [prom]   Play a move using square->square notation
    // 5. setup [FEN]               Only when game isn't currently running: initialize the board with a assumed-well-formed FEN
    // 6. setup                     Enters their setup mode; redraws after every command
    //   6.1 + [piece] [sq]         Place [piece] at [sq] on top of any other piece that's there
    //   6.2 - [sq]                 Remove any piece on [sq]
    //   6.3 = [black||white]       Makes it [black||white] 's turn to play
    //   6.4 done                   Before exiting, verify: 1 of each king, pawns in legal spots, no king in check
    // ----- OUR NEW SETTINGS ------
    // 7. help                      Prints out documentation for all the commands
    // 8. undo                      Undos the most recent move
    // 9. score                     Prints the score
    // 10. settings                 Tells the user which settings correspond to what integers
    // 11. toggle [int]             Toggle the setting at [int] (0 <= x)
    // 12. perft [int]              Does a Perft test with [int]

    std::pair<int, int> scores = {0, 0}; // {white, black}

    std::string currVal;

    Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::cout << " ● Type a command: ";

    while (std::cin >> currVal) {
        if (currVal == "game") {
            // TODO with alex
        } else if (currVal == "resign") {
            // TODO with alex
        } else if (currVal == "move") {
            // TODO with alex
        } else if (currVal == "setup") {
            // TODO with alex
        } else if (currVal == "help" || currVal == "man") {
            std::cout << " ◌ ╭─────────────────────────────────────╮" << std::endl;
            std::cout << " ◌ │ HAGNUS MIEMAN CHESS ENGINE - Manual │" << std::endl;
            std::cout << " ◌ ╰─────────────────────────────────────╯" << std::endl;
            std::cout << " ◌ ╭──╴" << std::endl;
            std::cout << " ◌ │ game [white] [black]" << std::endl;
            std::cout << " ◌ │         Options are `player` and `computer[1-4]`." << std::endl;
            std::cout << " ◌ │ man" << std::endl;
            std::cout << " ◌ │         Opens this manual. `help` also does." << std::endl;
            std::cout << " ◌ │ move" << std::endl;
            std::cout << " ◌ │         Tells the computer to play its move." << std::endl;
            std::cout << " ◌ │ move [from] [to] [promotion]" << std::endl;
            std::cout << " ◌ │         Play a move, for example: `move g2 g1 R`." << std::endl;
            std::cout << " ◌ │ perft [0-50]" << std::endl;
            std::cout << " ◌ │         Run a PERFT test on the current board." << std::endl;
            std::cout << " ◌ │ resign" << std::endl;
            std::cout << " ◌ │         Resigns the current game." << std::endl;
            std::cout << " ◌ │ score" << std::endl;
            std::cout << " ◌ │         Display the current scores of White and Black players." << std::endl;
            std::cout << " ◌ │ settings" << std::endl;
            std::cout << " ◌ │         Display the current settings." << std::endl;
            std::cout << " ◌ │ setup [FEN]" << std::endl;
            std::cout << " ◌ │         Initialize a game with a well-formed FEN." << std::endl;
            std::cout << " ◌ │ setup" << std::endl;
            std::cout << " ◌ │         Enters setup mode, with these methods:" << std::endl;
            std::cout << " ◌ │         + [piece] [at]" << std::endl;
            std::cout << " ◌ │                 Place `piece` at square `at`, on top of whatever is there." << std::endl;
            std::cout << " ◌ │         - [at]" << std::endl;
            std::cout << " ◌ │                 Remove any piece at square `at`." << std::endl;
            std::cout << " ◌ │         = [colour]" << std::endl;
            std::cout << " ◌ │                 Make it `colour`'s turn to play. Can be `white` or `black`." << std::endl;
            std::cout << " ◌ │         done" << std::endl;
            std::cout << " ◌ │                 Exit setup mode, if restrictions are met." << std::endl;
            std::cout << " ◌ │ toggle [0-3]" << std::endl;
            std::cout << " ◌ │         Toggle the numbered setting." << std::endl;
            std::cout << " ◌ │ undo" << std::endl;
            std::cout << " ◌ │         Undoes the previous move in the current game." << std::endl;
            std::cout << " ◌ ╰──╴" << std::endl;
        } else if (currVal == "undo") {
            // TODO with alex
        } else if (currVal == "score") {
            std::cout << " ◌ Current Scores:" << std::endl;
            std::cout << " ◌ White: " << scores.first << std::endl;
            std::cout << " ◌ Black: " << scores.second << std::endl;
        } else if (currVal == "settings" || currVal == "setting") {
            std::cout << " ◌ Type `toggle [0-3]` to toggle these settings:" << std::endl;
            std::cout << " ◌ 0 - ASCII pieces        " << (io.getSetting(0) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 1 - Checkerboard        " << (io.getSetting(1) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 2 - Flip perspective    " << (io.getSetting(2) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 3 - Wide display        " << (io.getSetting(3) ? "ON" : "OFF") << std::endl;
        } else if (currVal == "toggle") {
            int n = -1;
            std::cin >> n;
            if (std::cin && n >= 0 && n <= 3) {
                io.toggleSetting(n);
            } else {
                std::cout << " ◌ Usage:      toggle [0-3]" << std::endl;
                std::cout << " ◌ Type `settings` for the setting list." << std::endl;
                std::cin.clear();
                std::cin.ignore();
            }
        } else if (currVal == "perft") {
            int n = -1;
            std::cin >> n;
            if (std::cin && n >= 0 && n <= 50) {
                board.perftTest(n);
            } else {
                std::cout << " ◌ Usage:      perft [0-50]" << std::endl;
                std::cin.clear();
                std::cin.ignore();
            }
        } else { // Invalid command
            std::cout << " ◌ " << currVal << " is not a command. Type `man` for the manual." << std::endl;
            std::cin.clear();
            std::cin.ignore();
        }
        std::cout << " ● Type a command: ";
    }
}
