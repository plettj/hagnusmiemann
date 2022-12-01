#include "board.h"
#include "move.h"
#include "io.h"
#include "constants.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <cmath>

int main() {
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
    // 13. print                    Prints the current board state

    std::pair<double, double> scores = {0, 0}; // {white, black}
    std::pair<int, int> players = {0, 0}; // 0: player.   1-4: computer[1-4]

    bool isGameRunning = false;

    int totalGames = 0;

    GameState state = Neutral;

    std::string currLine;

    Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::cout << std::endl;
    std::cout << "  ╭────────────────────────────────────────────────────────────────────────────╮" << std::endl;
    std::cout << "  │       __  __    ______    ______    __   __    __  __    ______            │" << std::endl;
    std::cout << "  │      /\\ \\_\\ \\  /\\  __ \\  /\\  ___\\  /\\ \"-.\\ \\  /\\ \\/\\ \\  /\\  ___\\           │" << std::endl;
    std::cout << "  │      \\ \\  __ \\ \\ \\  __ \\ \\ \\ \\__-\\ \\ \\ \\-.  \\ \\ \\ \\_\\ \\ \\ \\___  \\          │" << std::endl;
    std::cout << "  │       \\ \\_\\ \\_\\ \\ \\_\\ \\_\\ \\ \\_____\\ \\ \\_\\\\\"\\_\\ \\ \\_____\\ \\/\\_____\\         │" << std::endl;
    std::cout << "  │        \\/_/\\/_/  \\/_/\\/_/  \\/_____/  \\/_/ \\/_/  \\/_____/  \\/_____/         │" << std::endl;
    std::cout << "  │   __    __    __    ______    __    __    ______    __   __    __   __     │" << std::endl;
    std::cout << "  │  /\\ \"-./  \\  /\\ \\  /\\  ___\\  /\\ \"-./  \\  /\\  __ \\  /\\ \"-.\\ \\  /\\ \"-.\\ \\    │" << std::endl;
    std::cout << "  │  \\ \\ \\-./\\ \\ \\ \\ \\ \\ \\  __\\  \\ \\ \\-./\\ \\ \\ \\  __ \\ \\ \\ \\-.  \\ \\ \\ \\-.  \\   │" << std::endl;
    std::cout << "  │   \\ \\_\\ \\ \\_\\ \\ \\_\\ \\ \\_____\\ \\ \\_\\ \\ \\_\\ \\ \\_\\ \\_\\ \\ \\_\\\\\"\\_\\ \\ \\_\\\\\"\\_\\  │" << std::endl;
    std::cout << "  │    \\/_/  \\/_/  \\/_/  \\/_____/  \\/_/  \\/_/  \\/_/\\/_/  \\/_/ \\/_/  \\/_/ \\/_/  │" << std::endl;
    std::cout << "  │      ___   _                        ___                 _                  │" << std::endl;
    std::cout << "  │     / __| | |_    ___   ___  ___   | __|  _ _    __ _  (_)  _ _    ___     │" << std::endl;
    std::cout << "  │    | (__  | ' \\  / -_) (_-< (_-<   | _|  | ' \\  / _` | | | | ' \\  / -_)    │" << std::endl;
    std::cout << "  │     \\___| |_||_| \\___| /__/ /__/   |___| |_||_| \\__, | |_| |_||_| \\___|    │" << std::endl;
    std::cout << "  |                                                 |___/                      |" << std::endl;
    std::cout << "  ╰────────────────────────────────────────────────────────────────────────────╯" << std::endl;

    std::cout << std::endl;
    std::cout << " ◌ Type `help` for a list of commands." << std::endl;
    std::cout << " ● Command: ";

    while (std::getline(std::cin, currLine)) {
        std::string command;
        std::istringstream lineStream{currLine};
        lineStream >> command;

        if (command == "game") {
            std::string first;
            lineStream >> first;
            std::string second;
            lineStream >> second;

            if (!isGameRunning && std::regex_match(first, std::regex("^(player)|((computer)[1-4])$")) && std::regex_match(second, std::regex("^(player)|((computer)[1-4])$"))) {
                players.first = first == "player" ? 0 : first[8] - '0';
                players.second = second == "player" ? 0 : second[8] - '0';
                std::cout << "╭──────────────────────────────────────" << (totalGames + 1 > 9 ? "─" : "") << "╮" << std::endl;
                std::cout << "│ HAGNUS MIEMANN CHESS ENGINE - Game " << totalGames + 1 << " │" << std::endl;
                std::cout << "├───────────────────┬──────────────────" << (totalGames + 1 > 9 ? "─" : "") << "┤" << std::endl;
                std::cout << "│ White: " << first << (players.first ? "" : "   ") << "  │ Black: " << second << (players.second ? "" : "   ") << (totalGames + 1 > 9 ? " " : "") << " │" << std::endl;
                std::cout << "╰───────────────────┴──────────────────" << (totalGames + 1 > 9 ? "─" : "") << "╯" << std::endl;
                totalGames++;
                isGameRunning = true;
                io.display(board, state);
            } else if (!isGameRunning) {
                if (second == "") {
                    std::cout << " ◌ Usage:      game [white] [black]" << std::endl;
                } else {
                    std::cout << " ◌ Malformed side names. Each must be `player` or `computer[1-4]`" << std::endl;
                }
            } else {
                std::cout << " ◌ A game is already in progress." << std::endl;
            }
        } else if (command == "resign") {
            if (!isGameRunning) {
                std::cout << " ◌ No game is currently in progress." << std::endl;
            } else {
                Color turn = board.getTurn();
                io.display(board, static_cast<GameState>(turn + 1));
                if (turn) scores.second++;
                else scores.first++;
                isGameRunning = false;
                board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            }
        } else if (command == "move") {
            std::string first = "";
            lineStream >> first;
            std::string second = "";
            lineStream >> second;
            std::string prom = "";
            lineStream >> prom;

            if (isGameRunning) {
                if (first == "") { // Command `move` with no parameters. Do computer move!
                    if ((players.first && !board.getTurn()) || (players.second && board.getTurn())) {

                    // TODO: code computer move functionality with Alex!

                    } else {
                        std::cout << " ◌ It's a player's turn. Specify the move." << std::endl;
                        std::cout << " ◌ Usage:      move [from] [to] [promotion?]" << std::endl;
                    }
                } else if (second == "") {
                    std::cout << " ◌ Usage:      move" << std::endl;
                    std::cout << " ◌ or          move [from] [to] [promotion?]" << std::endl;
                } else if ((!players.first && !board.getTurn()) || (!players.second && board.getTurn())) {
                    if (std::regex_match(first, std::regex("^[a-h][1-8]$")) && std::regex_match(first, std::regex("^[a-h][1-8]$"))) {
                        if (first != second) {
                            Square from = Board::squareFromString(first);
                            Square to = Board::squareFromString(second);

                            Piece promPiece = Piece::Knight;

                            ColorPiece piece = board.getPieceAt(from);

                            if (piece != ColorPiece::Empty) {
                                Move::MoveType type = Move::Normal;

                                if (piece == ColorPiece::BlackKing || piece == ColorPiece::WhiteKing) { // King move.
                                    int fileDistance = std::abs(first[0] - second[0]);
                                    int rankDistance = std::abs(first[1] - second[1]);
                                    if (fileDistance >= 2 && fileDistance <= 4 && rankDistance == 0 && (first[1] == '1' || first[1] == '8') && first[0] == 'e') {
                                        // Our system accepts both their notation AND ours!
                                        // Even `e1 b1` can cause queenside castling. This is intentional.
                                        type = Move::Castle;

                                        if (second[1] > 'e') second[1] = 'h';
                                        else second[1] = 'a';
                                        to = Board::squareFromString(second);

                                    } else if (fileDistance > 1 || rankDistance > 1) {
                                        std::cout << " ◌ The king can't move that far, unless it's castling." << std::endl;
                                        std::cout << " ● Command: ";
                                        continue; // To stop the code from trying to play the move.
                                    }
                                } else if (piece == ColorPiece::BlackPawn || piece == ColorPiece::WhitePawn) { // Pawn move.
                                    Square passant = board.getEnpassantSquare();
                                    if (passant == to) {
                                        type = Move::Enpassant;
                                    } else if (second[1] == '1' || second[1] == '8') {
                                        type = Move::Promotion;

                                        if (prom == "Q") promPiece = Piece::Queen;
                                        else if (prom == "R") promPiece = Piece::Rook;
                                        else if (prom == "B") promPiece = Piece::Rook;
                                        else if (prom == "N") promPiece = Piece::Rook;
                                        else {
                                            std::cout << " ◌ Usage:      move [from] [to] [promotion]" << std::endl;
                                            std::cout << " ◌ A valid promotion piece (Q, R, B, N) was not specified." << std::endl;
                                            std::cout << " ● Command: ";
                                            continue; // To stop the code from trying to play the move.
                                        }
                                    }
                                }
                                
                                Move move = Move{from, to, type, promPiece};
                                bool pseudoLegal = board.isMovePseudoLegal(move);

                                if (pseudoLegal) {
                                    bool fullyLegal = board.applyMove(move);
                                    if (fullyLegal) {
                                        //Color turn = board.getTurn();

                                        // TODO: With alex, update `state` based on the result of the game

                                        io.display(board, state);
                                    } else {
                                        std::cout << " ◌ This move leaves you in check." << std::endl;
                                    }
                                } else {
                                    if (type == Move::Castle) {
                                        std::cout << " ◌ This castling move is not legal." << std::endl;
                                    } else if (type == Move::Promotion) {
                                        std::cout << " ◌ This pawn promotion is not legal." << std::endl;
                                    } else if (type == Move::Enpassant) {
                                        std::cout << " ◌ You found an illegal en passant move! Nice." << std::endl;
                                    } else {
                                        std::cout << " ◌ The piece on " << first << " can't move to " << second << "." << std::endl;
                                    }
                                }
                            } else {
                                std::cout << " ◌ There is no piece on " << first << "." << std::endl;
                            }
                        } else {
                            std::cout << " ◌ The squares cannot be the same." << std::endl;
                        }
                    } else {
                        std::cout << " ◌ One of the your squares, " << first << " and " << second << ", is malformed." << std::endl;
                    }
                } else {
                    std::cout << " ◌ It's a computer's turn. Just type `move` to make it play." << std::endl;
                }
            } else if (isGameRunning) {
                std::cout << " ◌ Usage:      move [from] [to] [promotion?]" << std::endl;
            } else {
                std::cout << " ◌ No game is currently in progress." << std::endl;
            }
        } else if (command == "setup") {
            // TODO with Alex
            //Basically just use board.setSquare(color, piece, square), board.clearSquare(square), board.setTurn(turn)
            //board.setCastlingRight(side, isKingside), board.setEnpassantSquare(square). The latter two
            //strongly assume that the correct preconditions are met (i.e. a rook actually exists in the correct place/a pawn actually is in an enpassant'able position)
            //also, setSquare will not yell at you if a pawn is placed on an illegal rank, check it beforehand
        } else if (command == "help" || command == "man") {
            std::cout << " ◌ ╭──────────────────────────────────────╮" << std::endl;
            std::cout << " ◌ │ HAGNUS MIEMANN CHESS ENGINE - Manual │" << std::endl;
            std::cout << " ◌ ╰──────────────────────────────────────╯" << std::endl;
            std::cout << " ◌ ╭──╴" << std::endl;
            std::cout << " ◌ │ exit" << std::endl;
            std::cout << " ◌ │         Immediately terminate the program." << std::endl;
            std::cout << " ◌ │ game [white] [black]" << std::endl;
            std::cout << " ◌ │         Options are `player` and `computer[1-4]`." << std::endl;
            std::cout << " ◌ │ help" << std::endl;
            std::cout << " ◌ │         Opens this manual. `man` also does." << std::endl;
            std::cout << " ◌ │ make" << std::endl;
            std::cout << " ◌ │         Just here to catch programmers who forgot to CTRL+C!" << std::endl;
            std::cout << " ◌ │ move" << std::endl;
            std::cout << " ◌ │         Tells the computer to play its move." << std::endl;
            std::cout << " ◌ │ move [from] [to] [promotion?]" << std::endl;
            std::cout << " ◌ │         Play a move. For example: `move e1 g1` or `move g2 g1 R`." << std::endl;
            std::cout << " ◌ │ perft [0-50]" << std::endl;
            std::cout << " ◌ │         Run a PERFT test on the current board." << std::endl;
            std::cout << " ◌ │ print" << std::endl;
            std::cout << " ◌ │         Display the current game." << std::endl;
            std::cout << " ◌ │ quit" << std::endl;
            std::cout << " ◌ │         Display the final scores, and exit the program." << std::endl;
            std::cout << " ◌ │ resign" << std::endl;
            std::cout << " ◌ │         Resigns the current game." << std::endl;
            std::cout << " ◌ │ scores" << std::endl;
            std::cout << " ◌ │         Display the current scores of White and Black players." << std::endl;
            std::cout << " ◌ │ settings" << std::endl;
            std::cout << " ◌ │         Display the current settings." << std::endl;
            std::cout << " ◌ │ setup [FEN]" << std::endl;
            std::cout << " ◌ │         Initialize a game with a well-formed FEN." << std::endl;
            std::cout << " ◌ │ setup" << std::endl;
            std::cout << " ◌ │         Enters setup mode, which has the following methods:" << std::endl;
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
        } else if (command == "undo") {
            if (isGameRunning) {
                int plies = board.getPlies();

                // TODO: plies is all bad! oh no.

                if (plies) {
                    board.revertMostRecent();
                    io.display(board, GameState::Neutral);
                } else {
                    std::cout << " ◌ The game has no moves to undo." << std::endl;
                }
            } else {
                std::cout << " ◌ No game is currently in progress." << std::endl;
            }
        } else if (command == "score" || command == "scores") {
            std::cout << " ◌ ╔═ Current Scores ═╗" << std::endl;
            std::cout << " ◌ ║ White: " << scores.first << (scores.first >= 10 ? "" : " ") << (scores.first - std::floor(scores.first) ? "" : "  ") << "      ║" << std::endl;
            std::cout << " ◌ ║ Black: " << scores.second << (scores.second >= 10 ? "" : " ") << (scores.second - std::floor(scores.second) ? "" : "  ") << "      ║" << std::endl;
            std::cout << " ◌ ╚══════════════════╝" << std::endl;
        } else if (command == "setting" || command == "settings") {
            std::cout << " ◌ Type `toggle [0-4]` to toggle these settings:" << std::endl;
            std::cout << " ◌ 0 - ASCII pieces        " << (!io.getSetting(0) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 1 - Checkerboard        " << (io.getSetting(1) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 2 - Flip perspective    " << (io.getSetting(2) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 3 - Wide display        " << (io.getSetting(3) ? "ON" : "OFF") << std::endl;
            std::cout << " ◌ 4 - Computer auto-move  " << (io.getSetting(4) ? "ON" : "OFF") << std::endl;
        } else if (command == "toggle") {
            int n = -1;
            lineStream >> n;
            if (lineStream && n >= 0 && n <= 4) {
                io.toggleSetting(n);
            } else {
                std::cout << " ◌ Usage:      toggle [0-4]" << std::endl;
                std::cout << " ◌ Type `settings` for the setting list." << std::endl;
            }
        } else if (command == "perft") {
            int n = -1;
            lineStream >> n;
            if (lineStream && n >= 0 && n <= 50) {
                board.perftTest(n);
            } else {
                std::cout << " ◌ Usage:      perft [0-50]" << std::endl;
            }
        } else if (command == "make") {
            std::cout << " ◌ You forgot to CTRL+C, you dingbat." << std::endl;
        } else if (command == "quit") {
            break;
        } else if (command == "exit") {
            return 0;
        } else if (command == "print") {
            if (isGameRunning) {
                io.display(board, state);
            } else {
                std::cout << " ◌ No game is currently in progress." << std::endl;
            }
        } else { // Invalid command
            std::cout << " ◌ `" << command << "` is not a command. Type `help` for the manual." << std::endl;
        }
        std::cout << " ● Command: ";
    }
    if (std::cin.eof()) std::cout << "Quitting..." << std::endl;
    std::cout << " ◌ " << std::endl;
    std::cout << " ◌ ╔══ Final Scores ══╗" << std::endl;
    std::cout << " ◌ ║ White: " << scores.first << (scores.first >= 10 ? "" : " ") << (scores.first - std::floor(scores.first) ? "" : "  ") << "      ║" << std::endl;
    std::cout << " ◌ ║ Black: " << scores.second << (scores.second >= 10 ? "" : " ") << (scores.second - std::floor(scores.second) ? "" : "  ") << "      ║" << std::endl;
    std::cout << " ◌ ╚══════════════════╝" << std::endl;
    std::cout << " ◌ " << std::endl;
    std::cout << " ◌ Thanks for using the Hagnus Miemann Chess Engine!" << std::endl << std::endl;
}
