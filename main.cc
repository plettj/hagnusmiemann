#include "board.h"
#include "move.h"
#include "io.h"

int main() {
  Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  std::cout << board.perftTest(3) << std::endl;
  //TextDisplay display = TextDisplay{std::cout};

  //display.printBoard(board);
}
