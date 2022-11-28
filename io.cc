#include "io.h"

void TextDisplay::printBoard(Board& board) {
    Board::Square kingSquare = board.getKing();
    Board::Color turn = board.getTurn();

    bool gameOver = false; // TODO!

    bool checked = board.isSquareAttacked(kingSquare, turn);

    std::string gameMessage[2] = {"White won by     │", "resignation.     │"};

    bool blackPerspective = (boardPerspective && static_cast<bool>(turn));

    out << "   " << "╔═════════════════" << ((wideBoard) ? "═══════" : "") << (basicPieces && !showCheckers ? "" : "═") << "╗" << std::endl;
    for (int rank = 0; rank < Board::NumRanks; ++rank) {
        int realRank = (blackPerspective) ? rank : 7 - rank;

        out << " " << realRank + 1 << " ║" << ((wideBoard) ? "" : " ");

        for (int file = 0; file < Board::NumFiles; ++file) {
            int realFile = (blackPerspective) ? 7 - file : file;

            Board::Square square = Board::getSquare(realRank, realFile);
            Board::ColorPiece piece = board.getPieceAt(square);
            int pieceInt = piece / 4 * 2 + piece % 2;
            if (pieceInt >= 12) { // blank square
                if (showCheckers) {
                    if ((realRank + realFile) % 2) {
                        out << ((wideBoard) ? " " : "") << "▓▓";
                    } else {
                        out << ((wideBoard) ? " " : "") << "░░";
                    }
                } else {
                    out << ((wideBoard) ? " " : "") << ((basicPieces) ? "· " : "╶╴");
                }
            } else if (basicPieces) {
                out << ((wideBoard) ? " " : "") << PieceChar[pieceInt] << " ";
            } else {
                out << ((wideBoard) ? " " : "") << PieceImage[pieceInt] << " ";
            }
        }

        out << (basicPieces && !showCheckers ? "" : " ") << "║";
        if (gameOver) {
            switch (rank) {
                case 2:
                    out << " ╭──────────────────╮"; break;
                case 3: case 4:
                    out << " │ " << gameMessage[rank - 3]; break;
                case 5:
                    out << " ╰──────────────────╯";
            }
        } else if (rank == 6 && checked && !gameOver) {
            out << "   ⟐  " << (turn ? "Black" : "White") << " is in check.";
        }
        out << std::endl;
    }

    out << "   " << "╚═════════════════" << ((wideBoard) ? "═══════" : "") << (basicPieces && !showCheckers ? "" : "═") << "╝";
    if (!gameOver) {
        out << "   ⟐  " << (turn ? "Black" : "White") << " to move.";
    }
    out << std::endl << "   " << (wideBoard ? "" : " ");

    for (int file = 0; file < Board::NumFiles; ++file) {
        int realFile = (blackPerspective) ? 7 - file : file;
        out << " " << ((wideBoard) ? " " : "") << static_cast<char>(realFile + 97);
    }

    out << std::endl;

}

void TextDisplay::setBasicPieces(bool basic) { basicPieces = basic; }
void TextDisplay::setShowCheckers(bool checkers) { showCheckers = checkers; }
void TextDisplay::setBoardPerspective(bool perspective) { boardPerspective = perspective; }
void TextDisplay::setWideBoard(bool wide) { wideBoard = wide; }

