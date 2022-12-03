#include "board.h"
#include "move.h"
#include "io.h"
#include "constants.h"
#include "difficultylevel.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <cmath>

IO::IO(std::istream& in, std::ostream& out): out{out} {
    input = new TextInput{in};
}
void IO::makeTextOutput(std::ostream& out) {
    TextOutput* textOutput = new TextOutput{input, out};
    outputs.push_back(textOutput);
}
void IO::toggleSetting(int setting) {
    switch (setting) {
        case 0:
            basicPieces = !basicPieces;
            break;
        case 1:
            showCheckers = !showCheckers;
            break;
        case 2:
            boardPerspective = !boardPerspective;
            break;
        case 3: 
            wideBoard = !wideBoard;
            break;
        case 4:
            autoMove = !autoMove;
            break;
    }
}
bool IO::getSetting(int setting) {
    switch (setting) {
        case 0:
            return basicPieces;
        case 1:
            return showCheckers;
        case 2:
            return boardPerspective;
        case 3: 
            return wideBoard;
        case 4:
            return autoMove;
        default:
            return false;
    }
}
void IO::makeGraphicOutput() {
    // TODO
}
void IO::display(Board& board, GameState state, bool setup) {
    input->notifyOutputs(board, std::array<bool, 4>{basicPieces, showCheckers, boardPerspective, wideBoard}, state, setup);
}
void IO::runProgram() {
    input->runProgram(*this, out);
}

TextOutput::TextOutput(Input* toFollow, std::ostream& out): Output{toFollow}, out{out} {
    toFollow->attach(this);
}
void TextOutput::display(Board& board, std::array<bool, 4> settings, GameState state, bool setup) {
    Square kingSquare = board.getKing();
    Color turn = board.getTurn();

    bool checked = board.isSquareAttacked(kingSquare, turn);

    std::string gameMessage[2] = {"", ""};
    switch (state) {
        case WhiteResigned:
            gameMessage[0] = "Black won by      │";
            gameMessage[1] = "resignation.      │";
            break;
        case BlackResigned:
            gameMessage[0] = "White won by      │";
            gameMessage[1] = "resignation.      │";
            break;
        case WhiteGotMated:
            gameMessage[0] = "Black won by      │";
            gameMessage[1] = "checkmate!        │";
            break;
        case BlackGotMated:
            gameMessage[0] = "White won by      │";
            gameMessage[1] = "checkmate!        │";
            break;
        case Stalemate:
            gameMessage[0] = "Game drawn by     │";
            gameMessage[1] = "stalemate.        │";
            break;
        case FiftyMove:
            gameMessage[0] = "Game drawn by you being so darn slow at whatever the heck you were trying to   │";
            gameMessage[1] = "do that you played 50 non-permanent moves in a row, you absolute *hand towel*. │";
            break;
        case Threefold:
            gameMessage[0] = "Game drawn by     │";
            gameMessage[1] = "3-fold repetition.│";
            break;
        case InsufficientMaterial:
            gameMessage[0] = "Game drawn by     │";
            gameMessage[1] = "scant material.   │";
            break;
        default:
            break;
    }

    std::string lastMoveString = "";
    Move lastMove = board.getLastPlayedMove();
    int plies = board.getTotalPlies();

    bool blackPerspective = (settings[2] && static_cast<bool>(turn));

    out << (setup ? " ◌ ╰╮   " : "   ") << "╔═════════════════" << ((settings[3]) ? "═══════" : "") << (settings[0] && !settings[1] ? "" : "═") << "╗";
    if (plies > 1 && !state) {
        out << "   ◈  " << (plies / 2) << ". " << (turn ? "" : "... ") << lastMove.toString() << (checked ? "+" : "");
    }
    out << std::endl;

    for (int rank = 0; rank < NumRanks; ++rank) {
        int realRank = (blackPerspective) ? rank : 7 - rank;

        out << (setup ? " ◌  │ " : " ") << realRank + 1 << " ║" << ((settings[3]) ? "" : " ");

        for (int file = 0; file < NumFiles; ++file) {
            int realFile = (blackPerspective) ? 7 - file : file;

            Square square = Board::getSquare(realRank, realFile);
            ColorPiece piece = board.getPieceAt(square);
            int pieceInt = piece / 4 * 2 + piece % 2;
            if (pieceInt >= 12) { // blank square
                if (settings[1]) {
                    if ((realRank + realFile) % 2) {
                        out << ((settings[3]) ? " " : "") << "▓▓";
                    } else {
                        out << ((settings[3]) ? " " : "") << "░░";
                    }
                } else {
                    out << ((settings[3]) ? " " : "") << ((settings[0]) ? "· " : "╶╴");
                }
            } else if (settings[0]) {
                out << ((settings[3]) ? " " : "") << PieceChar[pieceInt] << " ";
            } else {
                out << ((settings[3]) ? " " : "") << PieceImage[pieceInt] << " ";
            }
        }

        out << (settings[0] && !settings[1] ? "" : " ") << "║";
        if (state) {
            switch (rank) {
                case 2:
                    out << " ╭───────────────────" << (state == GameState::FiftyMove ? "─────────────────────────────────────────────────────────────" : "") << "╮"; break;
                case 3: case 4:
                    out << " │ " << gameMessage[rank - 3]; break;
                case 5:
                    out << " ╰───────────────────" << (state == GameState::FiftyMove ? "─────────────────────────────────────────────────────────────" : "") << "╯";
            }
        } else {
            if (rank == 6 && checked) {
                out << "   ◈  " << (turn ? "Black" : "White") << " is in check.";
            }
        }
        out << std::endl;
    }

    out << (setup ? " ◌  │   " : "   ") << "╚═════════════════" << ((settings[3]) ? "═══════" : "") << (settings[0] && !settings[1] ? "" : "═") << "╝";
    if (!state) {
        out << "   ◈  " << (turn ? "Black" : "White") << " to move.";
    }
    out << std::endl << (setup ? " ◌ ╭╯   " : "   ") << (settings[3] ? "" : " ");

    for (int file = 0; file < NumFiles; ++file) {
        int realFile = (blackPerspective) ? 7 - file : file;
        out << " " << ((settings[3]) ? " " : "") << static_cast<char>(realFile + 97);
    }

    out << std::endl;

}

GraphicalOutput::GraphicalOutput(Input* toFollow): Output{toFollow} {
    toFollow->attach(this);
}
void GraphicalOutput::display(Board& board, std::array<bool, 4> settings, GameState state, bool setup) {
    // TODO
}

void TextInput::attach(Output* output) {
    outputs.emplace_back(output);
}
void TextInput::detach(Output* output) {
    // Run through the outputs [observers] until we find the matching Output*.
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (*it == output) {
            outputs.erase(it);
            break;
        }
    }
}
void TextInput::notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup) {
    for (auto out : outputs) out->display(board, settings, state, setup);
}
void TextInput::runProgram(IO& io, std::ostream& out) {

    io.makeTextOutput(out); // This is a required part of our input interface.

    std::pair<double, double> scores = {0, 0}; // {white, black}
    std::pair<int, int> players = {0, 0}; // 0: player.   1-4: computer[1-4]
    //DifficultyLevel* currCompPlayer;

    bool isGameRunning = false;

    int totalGames = 0;

    GameState state = Neutral;

    std::string currLine;

    Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    out << std::endl;
    out << "  ╭────────────────────────────────────────────────────────────────────────────╮" << std::endl;
    out << "  │       __  __    ______    ______    __   __    __  __    ______            │" << std::endl;
    out << "  │      /\\ \\_\\ \\  /\\  __ \\  /\\  ___\\  /\\ \"-.\\ \\  /\\ \\/\\ \\  /\\  ___\\           │" << std::endl;
    out << "  │      \\ \\  __ \\ \\ \\  __ \\ \\ \\ \\__-\\ \\ \\ \\-.  \\ \\ \\ \\_\\ \\ \\ \\___  \\          │" << std::endl;
    out << "  │       \\ \\_\\ \\_\\ \\ \\_\\ \\_\\ \\ \\_____\\ \\ \\_\\\\\"\\_\\ \\ \\_____\\ \\/\\_____\\         │" << std::endl;
    out << "  │        \\/_/\\/_/  \\/_/\\/_/  \\/_____/  \\/_/ \\/_/  \\/_____/  \\/_____/         │" << std::endl;
    out << "  │   __    __    __    ______    __    __    ______    __   __    __   __     │" << std::endl;
    out << "  │  /\\ \"-./  \\  /\\ \\  /\\  ___\\  /\\ \"-./  \\  /\\  __ \\  /\\ \"-.\\ \\  /\\ \"-.\\ \\    │" << std::endl;
    out << "  │  \\ \\ \\-./\\ \\ \\ \\ \\ \\ \\  __\\  \\ \\ \\-./\\ \\ \\ \\  __ \\ \\ \\ \\-.  \\ \\ \\ \\-.  \\   │" << std::endl;
    out << "  │   \\ \\_\\ \\ \\_\\ \\ \\_\\ \\ \\_____\\ \\ \\_\\ \\ \\_\\ \\ \\_\\ \\_\\ \\ \\_\\\\\"\\_\\ \\ \\_\\\\\"\\_\\  │" << std::endl;
    out << "  │    \\/_/  \\/_/  \\/_/  \\/_____/  \\/_/  \\/_/  \\/_/\\/_/  \\/_/ \\/_/  \\/_/ \\/_/  │" << std::endl;
    out << "  │      ___   _                        ___                 _                  │" << std::endl;
    out << "  │     / __| | |_    ___   ___  ___   | __|  _ _    __ _  (_)  _ _    ___     │" << std::endl;
    out << "  │    | (__  | ' \\  / -_) (_-< (_-<   | _|  | ' \\  / _` | | | | ' \\  / -_)    │" << std::endl;
    out << "  │     \\___| |_||_| \\___| /__/ /__/   |___| |_||_| \\__, | |_| |_||_| \\___|    │" << std::endl;
    out << "  │                                                 |___/                      │" << std::endl;
    out << "  ╰────────────────────────────────────────────────────────────────────────────╯" << std::endl;

    out << std::endl;
    out << " ◌ Type `help` for a list of commands." << std::endl;
    out << " ● Command: ";

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
                out << "╭──────────────────────────────────────" << (totalGames + 1 > 9 ? "─" : "") << "╮" << std::endl;
                out << "│ HAGNUS MIEMANN CHESS ENGINE - Game " << totalGames + 1 << " │" << std::endl;
                out << "├───────────────────┬──────────────────" << (totalGames + 1 > 9 ? "─" : "") << "┤" << std::endl;
                out << "│ White: " << first << (players.first ? "" : "   ") << "  │ Black: " << second << (players.second ? "" : "   ") << (totalGames + 1 > 9 ? " " : "") << " │" << std::endl;
                out << "╰───────────────────┴──────────────────" << (totalGames + 1 > 9 ? "─" : "") << "╯" << std::endl;
                totalGames++;
                isGameRunning = true;
                state = GameState::Neutral;
                io.display(board, state);
            } else if (!isGameRunning) {
                if (second == "") {
                    out << " ◌ Usage:  game [white] [black]" << std::endl;
                } else {
                    out << " ◌ Malformed side names. Each must be `player` or `computer[1-4]`" << std::endl;
                }
            } else {
                out << " ◌ A game is already in progress." << std::endl;
            }
        } else if (command == "resign") {
            if (!isGameRunning) {
                out << " ◌ No game is currently in progress." << std::endl;
            } else {
                Color turn = board.getTurn();
                if ((players.first && !turn) || (players.second && turn)) {
                    out << " ◌ You cannot make the computer resign." << std::endl;
                } else {
                    io.display(board, static_cast<GameState>(turn + 1));
                    if (turn) scores.first++;
                    else scores.second++;
                    isGameRunning = false;
                    state = GameState::Neutral;
                    board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                }
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
                        out << " ◌ We haven't implemented computer move functionality yet." << std::endl;

                    } else {
                        out << " ◌ It's a player's turn. Specify the move." << std::endl;
                        out << " ◌ Usage:  move [from] [to] [promotion?]" << std::endl;
                    }
                } else if (second == "") {
                    out << " ◌ Usage:  move" << std::endl;
                    out << " ◌ or          move [from] [to] [promotion?]" << std::endl;
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

                                        if (second[0] > 'e') second[0] = 'h';
                                        else second[0] = 'a';
                                        to = Board::squareFromString(second);

                                    } else if (fileDistance > 1 || rankDistance > 1) {
                                        out << " ◌ The king can't move that far, unless it's castling." << std::endl;
                                        out << " ● Command: ";
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
                                            out << " ◌ Usage:  move [from] [to] [promotion]" << std::endl;
                                            out << " ◌ A valid promotion piece (Q, R, B, N) was not specified." << std::endl;
                                            out << " ● Command: ";
                                            continue; // To stop the code from trying to play the move.
                                        }
                                    }
                                }
                                
                                Move move = Move{from, to, type, promPiece};
                                bool pseudoLegal = board.isMovePseudoLegal(move);

                                if (pseudoLegal) {
                                    bool fullyLegal = board.applyMove(move);
                                    if (fullyLegal) {
                                        Color turn = board.getTurn();

                                        if (!board.countLegalMoves() || board.isDrawn()) { // Game is over
                                            if (board.isSideInCheck(turn)) { // In Check
                                                if (turn) scores.first++;
                                                else scores.second++;
                                                state = static_cast<GameState>(turn + 3);
                                            } else {
                                                if (board.isInsufficientMaterialDraw()) state = GameState::InsufficientMaterial;
                                                else if (board.isFiftyMoveRuleDraw()) state = GameState::FiftyMove;
                                                else if (board.isThreefoldDraw()) state = GameState::Threefold;
                                                else state = GameState::Stalemate;
                                            }
                                            io.display(board, state);
                                            isGameRunning = false;
                                            state = GameState::Neutral;
                                            board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                                        } else {
                                            io.display(board, state);
                                        }
                                    } else {
                                        out << " ◌ This move leaves you in check." << std::endl;
                                    }
                                } else {
                                    if (type == Move::Castle) {
                                        out << " ◌ This castling move is not legal." << std::endl;
                                    } else if (type == Move::Promotion) {
                                        out << " ◌ This pawn promotion is not legal." << std::endl;
                                    } else if (type == Move::Enpassant) {
                                        out << " ◌ You found an illegal en passant move! Nice." << std::endl;
                                    } else {
                                        out << " ◌ The piece on " << first << " can't move to " << second << "." << std::endl;
                                    }
                                }
                            } else {
                                out << " ◌ There is no piece on " << first << "." << std::endl;
                            }
                        } else {
                            out << " ◌ The squares cannot be the same." << std::endl;
                        }
                    } else {
                        out << " ◌ One of the your squares, " << first << " and " << second << ", is malformed." << std::endl;
                    }
                } else {
                    out << " ◌ It's a computer's turn. Just type `move` to make it play." << std::endl;
                }
            } else if (isGameRunning) {
                out << " ◌ Usage:  move [from] [to] [promotion?]" << std::endl;
            } else {
                out << " ◌ No game is currently in progress." << std::endl;
            }
        } else if (command == "setup") {
            std::string first = "";
            lineStream >> first;

            if (!isGameRunning) {
                if (first == "") {
                    out << " ◌ ╭─────╴ SETUP MODE - Opened" << std::endl;
                    out << " ◌ ├╴ + [piece] [square]   Adds a piece." << std::endl;
                    out << " ◌ ├╴ - [square]           Removes a piece." << std::endl;
                    out << " ◌ ├╴ = [colour]           Make it `colour`'s turn to play." << std::endl;
                    // If we wanted to implement setting castling rights or en-passant squares, we would use:
                    // board.setCastlingRight(Color side, isKingside);     // returns true, unless it didn't do it.
                    // board.clearCastlingRight(Color side, isKingside);   // returns false.
                    // std::string board.getCastlingRights()               // returns fen string.
                    // board.setEnpassantSquare(square);                   
                    // BoardLegality board.getBoardLegalityState();        // returns legality state.
                    out << " ◌ ├╴ done                 Exits setup mode." << std::endl;
                    
                    board = Board::createBoardFromFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
                    io.display(board, state, true);

                    out << " ● │ Command: ";
                    while (std::getline(std::cin, currLine)) {
                        std::istringstream lineStream{currLine};
                        lineStream >> command;

                        if (command == "+") {
                            std::string first = "";
                            lineStream >> first;
                            std::string second = "";
                            lineStream >> second;

                            if (second != "") {
                                if (!std::regex_match(first, std::regex("^[pnbrqkPNBRQK]$"))) {
                                    out << " ◌ │ A valid piece (p, n, b, r, q, k, P, N, B, R, Q, K) was not specified." << std::endl;
                                } else if (!std::regex_match(second, std::regex("^[a-h][1-8]$"))){
                                    out << " ◌ │ A valid square (a1 through h8) was not specified." << std::endl;
                                } else {
                                    Color color = (first[0] > 96) ? Color::White : Color::Black;
                                    Piece piece = charToPiece((color ? first[0] : (first[0] - 32)));
                                    Square square = Board::squareFromString(second);
                                    board.clearSquare(square);
                                    board.setSquare(color, piece, square);
                                    io.display(board, state, true);
                                }
                            } else {
                                out << " ◌ │ Usage:  + [pnbrqkPNBRQK] [a-h][1-8]" << std::endl;
                            }
                        } else if (command == "-") {
                            std::string first = "";
                            lineStream >> first;

                            if (first != "") {
                                if (!std::regex_match(first, std::regex("^[a-h][1-8]$"))) {
                                    out << " ◌ │ A valid square (a1 through h8) was not specified." << std::endl;
                                } else {
                                    Square square = Board::squareFromString(first);
                                    board.clearSquare(square);
                                    io.display(board, state, true);
                                }
                            } else {
                                out << " ◌ │ Usage:  - [square]" << std::endl;
                            }
                        } else if (command == "=") {
                            std::string first = "";
                            lineStream >> first;

                            if (first != "") {
                                if (first == "black" || first == "white") {
                                    Color turn = (first == "white") ? Color::White : Color::Black;
                                    board.setTurn(turn);
                                    io.display(board, state, true);
                                } else {
                                    out << " ◌ │ A valid colour (black, white) was not specified." << std::endl;
                                }
                            } else {
                                out << " ◌ │ Usage:  = [colour]" << std::endl;
                            }
                        } else if (command == "done") {
                            // TODO: verify that board is "legal"!
                            break;
                        } else if (command != "") { // Invalid command
                            out << " ◌ │ `" << command << "` is not a command." << std::endl;
                        }
                        out << " ● │ Command: ";
                    }
                    if (std::cin.eof()) out << "Quitting..." << std::endl;
                    out << " ◌ ╰─────╴ SETUP MODE ─ Closed" << std::endl;
                } else {
                    // fen setup mode
                    std::string fen = currLine.substr(6);
                    if (std::regex_match(fen, std::regex("\\s*([rnbqkpRNBQKP1-8]+\\/){7}([rnbqkpRNBQKP1-8]+)\\s[bw-]\\s(([a-hkqA-HKQ]{1,4})|(-))\\s(([a-h][36])|(-))\\s\\d+\\s\\d+\\s*"))) {
                        out << " ◌ ╭─────╴ SETUP MODE ─ Opened" << std::endl;
                        board = Board::createBoardFromFEN(fen); // No error-checking here because official FEN correctness is a pain in the ass.
                        out << " ◌ │ Board successfully initialized with your FEN: " << std::endl;
                        io.display(board, state, true); // passing in `true` means IO displays it in setup mode.
                        out << " ◌ ╰─────╴ SETUP MODE ─ Closed" << std::endl;
                    } else {
                        out << " ◌ Your FEN was malformed." << std::endl;
                        out << " ◌ Usage:  setup [no parameters]" << std::endl;
                        out << " ◌         setup [FEN]" << std::endl;
                    }
                }
            } else {
                out << " ◌ A game is already in progress." << std::endl;
            }
        } else if (command == "help" || command == "man") {
            out << " ◌ ╭──────────────────────────────────────╮" << std::endl;
            out << " ◌ │ HAGNUS MIEMANN CHESS ENGINE - Manual │" << std::endl;
            out << " ◌ ╰──────────────────────────────────────╯" << std::endl;
            out << " ◌ ╭─────╴" << std::endl;
            out << " ◌ ├╴ exit" << std::endl;
            out << " ◌ │         Immediately terminates the program." << std::endl;
            out << " ◌ ├╴ game [white] [black]" << std::endl;
            out << " ◌ │         Starts a new game. Options are `player` and `computer[1-4]`." << std::endl;
            out << " ◌ ├╴ help" << std::endl;
            out << " ◌ │         Opens this manual. `man` also does." << std::endl;
            //out << " ◌ ├╴ make" << std::endl;
            //out << " ◌ │         Captures programmers who forgot to CTRL+C!" << std::endl;
            //out << " ◌ ├╴ ./chess" << std::endl;
            //out << " ◌ │         Captures programmers who have no short-term memory." << std::endl;
            out << " ◌ ├╴ move" << std::endl;
            out << " ◌ │         Tells the computer to compute and play its move." << std::endl;
            out << " ◌ ├╴ move [from] [to] [promotion?]" << std::endl;
            out << " ◌ │         Plays a move. For example: `move e1 g1` or `move g2 g1 R`." << std::endl;
            out << " ◌ ├╴ perft [0-15]" << std::endl;
            out << " ◌ │         Runs a PERFT test on the current board." << std::endl;
            out << " ◌ ├╴ print" << std::endl;
            out << " ◌ │         Displays the current game." << std::endl;
            out << " ◌ ├╴ quit" << std::endl;
            out << " ◌ │         Submits EOF; displays the final scores and exits the program." << std::endl;
            out << " ◌ ├╴ resign" << std::endl;
            out << " ◌ │         Resigns the current game." << std::endl;
            out << " ◌ ├╴ scores" << std::endl;
            out << " ◌ │         Displays the current scores of White and Black players." << std::endl;
            out << " ◌ ├╴ settings" << std::endl;
            out << " ◌ │         Displays the current settings." << std::endl;
            out << " ◌ ├╴ setup [FEN]" << std::endl;
            out << " ◌ │         Initializes a game with a well-formed FEN." << std::endl;
            out << " ◌ ├╴ setup" << std::endl;
            out << " ◌ ╰──╮      Enters setup mode, which has the following methods:" << std::endl;
            out << " ◌    ├╴ + [piece] [square]" << std::endl;
            out << " ◌    │          Places `piece` at square `square`, on top of whatever is there." << std::endl;
            out << " ◌    ├╴ - [square]" << std::endl;
            out << " ◌    │          Removes any piece at square `square`." << std::endl;
            out << " ◌    ├╴ = [colour]" << std::endl;
            out << " ◌    │          Makes it `colour`'s turn to play." << std::endl;
            out << " ◌    ├╴ done" << std::endl;
            out << " ◌ ╭──╯          Exits setup mode, if restrictions are met." << std::endl;
            out << " ◌ ├╴ toggle [0-3]" << std::endl;
            out << " ◌ │         Toggles the numbered setting." << std::endl;
            out << " ◌ ├╴ undo" << std::endl;
            out << " ◌ │         Undoes the previous move in the current game." << std::endl;
            out << " ◌ ╰─────╴" << std::endl;
        } else if (command == "undo") {
            if (isGameRunning) {
                if (board.getTotalPlies() > 1) {
                    board.revertMostRecent();
                    io.display(board, GameState::Neutral);
                } else {
                    out << " ◌ The game has no moves to undo." << std::endl;
                }
            } else {
                out << " ◌ No game is currently in progress." << std::endl;
            }
        } else if (command == "score" || command == "scores") {
            out << " ◌ ╔═ Current Scores ═╗" << std::endl;
            out << " ◌ ║ White: " << scores.first << (scores.first >= 10 ? "" : " ") << (scores.first - std::floor(scores.first) ? "" : "  ") << "      ║" << std::endl;
            out << " ◌ ║ Black: " << scores.second << (scores.second >= 10 ? "" : " ") << (scores.second - std::floor(scores.second) ? "" : "  ") << "      ║" << std::endl;
            out << " ◌ ╚══════════════════╝" << std::endl;
        } else if (command == "setting" || command == "settings") {
            out << " ◌ Type `toggle [0-4]` to toggle these settings:" << std::endl;
            out << " ◌ 0 - ASCII pieces        " << (!io.getSetting(0) ? "ON" : "OFF") << std::endl;
            out << " ◌ 1 - Checkerboard        " << (io.getSetting(1) ? "ON" : "OFF") << std::endl;
            out << " ◌ 2 - Flip perspective    " << (io.getSetting(2) ? "ON" : "OFF") << std::endl;
            out << " ◌ 3 - Wide display        " << (io.getSetting(3) ? "ON" : "OFF") << std::endl;
            out << " ◌ 4 - Computer auto-move  " << (io.getSetting(4) ? "ON" : "OFF") << std::endl;
        } else if (command == "toggle") {
            int n = -1;
            lineStream >> n;
            if (lineStream && n >= 0 && n <= 4) {
                io.toggleSetting(n);
            } else {
                out << " ◌ Usage:  toggle [0-4]" << std::endl;
                out << " ◌ Type `settings` for the setting list." << std::endl;
            }
        } else if (command == "perft") {
            int n = -1;
            lineStream >> n;
            if (lineStream && n >= 0 && n <= 15) {
                board.perftTest(n);
            } else {
                out << " ◌ Usage:  perft [0-15]" << std::endl;
            }
        } else if (command == "make") {
            out << " ◌ You forgot to CTRL+C, you dingbat." << std::endl;
        } else if (command == "./chess") {
            out << " ◌ You're already running the chess program, you muttonhead." << std::endl;
        } else if (command == "quit") {
            break;
        } else if (command == "exit") {
            return;
        } else if (command == "print") {
            if (isGameRunning) {
                io.display(board, state);
            } else {
                out << " ◌ No game is currently in progress." << std::endl;
            }
        } else if (command != "") { // Invalid command
            out << " ◌ `" << command << "` is not a command. Type `help` for the manual." << std::endl;
        }
        out << " ● Command: ";
    }
    if (std::cin.eof()) out << "Quitting..." << std::endl;
    out << " ◌ " << std::endl;
    out << " ◌ ╔══ Final Scores ══╗" << std::endl;
    out << " ◌ ║ White: " << scores.first << (scores.first >= 10 ? "" : " ") << (scores.first - std::floor(scores.first) ? "" : "  ") << "      ║" << std::endl;
    out << " ◌ ║ Black: " << scores.second << (scores.second >= 10 ? "" : " ") << (scores.second - std::floor(scores.second) ? "" : "  ") << "      ║" << std::endl;
    out << " ◌ ╚══════════════════╝" << std::endl;
    out << " ◌ " << std::endl;
    out << " ◌ Thanks for using the Hagnus Miemann Chess Engine!" << std::endl << std::endl;
}
