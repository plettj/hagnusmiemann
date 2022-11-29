#include "board.h"
#include "move.h"
#include "io.h"
#include <iostream>

int main() {
  Board board = Board::createBoardFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  board.perftTest(5);

  IO io{std::cin};
  io.makeTextOutput(std::cout);

  io.toggleSetting(0);
  io.toggleSetting(1);
  io.toggleSetting(3);

  io.display(board);
}
