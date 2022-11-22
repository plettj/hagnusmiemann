#include "io.h"

void TextDisplay::printBoard(Board& board) {
    // affect based on basicPieces
    // affect based on showCheckers
    // affect based on boardPerspective
    Board::Square kingSpot = board.getKing();
    Board::Color turn = board.getTurn();

    out << "   " << "╔" << (!basicPieces ? "═" : "") << "════════════════════════╗" << std::endl;
    for (int rank = Board::NumRanks - 1; rank >= 0; --rank) {
        out << " ║ ";
        for (int file = 0; file < Board::NumFiles; ++file) {
            std::cerr << rank << std::endl;
            Board::Square square = Board::getSquare(rank, file);
            Board::ColorPiece piece = board.getPieceAt(square);
            int pieceInt = piece / 4 + piece % 2;
            if (pieceInt == 12) { // blank square
                out << " ";
            } else if (basicPieces) {
                out << PieceChar[basicPieces];
            } else {
                out << PieceImage[basicPieces];
            }
        }
        out << " ║ " << std::endl;
    }
    out << "   " << "╚" << (!basicPieces ? "═" : "") << "════════════════════════╝" << std::endl;

}
