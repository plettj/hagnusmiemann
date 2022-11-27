#include "board.h"
#include "move.h"
#include "io.h"
#include <iostream>

int main() {
  Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  board.perftTest(7);
  
  TextDisplay display = TextDisplay{std::cout};
  display.setBasicPieces(false);
  display.setShowCheckers(false);
  display.setBoardPerspective(true);
  display.setWideBoard(false);
  display.printBoard(board);
}
