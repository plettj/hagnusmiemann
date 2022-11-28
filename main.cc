#include "board.h"
#include "move.h"
#include "io.h"
#include <iostream>

int main() {
  Board board = Board::createBoardFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  board.perftTest(5);
  
  TextDisplay display = TextDisplay{std::cout};
  display.setBasicPieces(false);
  display.setShowCheckers(false);
  display.setBoardPerspective(true);
  display.setWideBoard(false);
  display.printBoard(board);
}
