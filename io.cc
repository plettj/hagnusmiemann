#include "io.h"
#include "constants.h"

IO::IO(std::istream& in) {
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
        default:
            return false;
    }
}
void IO::makeGraphicOutput() {
    // TODO
}
void IO::display(Board& board) {
    input->notifyOutputs(board, std::array<bool, 4>{basicPieces, showCheckers, boardPerspective, wideBoard});
}

TextOutput::TextOutput(Input* toFollow, std::ostream& out): Output{toFollow}, out{out} {
    toFollow->attach(this);
}
void TextOutput::display(Board& board, std::array<bool, 4> settings) {
    Square kingSquare = board.getKing();
    Color turn = board.getTurn();

    bool gameOver = false; // TODO!

    bool checked = board.isSquareAttacked(kingSquare, turn);

    std::string gameMessage[2] = {"White won by     │", "resignation.     │"};

    bool blackPerspective = (settings[2] && static_cast<bool>(turn));

    out << "   " << "╔═════════════════" << ((settings[3]) ? "═══════" : "") << (settings[0] && !settings[1] ? "" : "═") << "╗" << std::endl;
    for (int rank = 0; rank < NumRanks; ++rank) {
        int realRank = (blackPerspective) ? rank : 7 - rank;

        out << " " << realRank + 1 << " ║" << ((settings[3]) ? "" : " ");

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
            out << "   ◈  " << (turn ? "Black" : "White") << " is in check.";
        }
        out << std::endl;
    }

    out << "   " << "╚═════════════════" << ((settings[3]) ? "═══════" : "") << (settings[0] && !settings[1] ? "" : "═") << "╝";
    if (!gameOver) {
        out << "   ◈  " << (turn ? "Black" : "White") << " to move.";
    }
    out << std::endl << "   " << (settings[3] ? "" : " ");

    for (int file = 0; file < NumFiles; ++file) {
        int realFile = (blackPerspective) ? 7 - file : file;
        out << " " << ((settings[3]) ? " " : "") << static_cast<char>(realFile + 97);
    }

    out << std::endl;

}

GraphicalOutput::GraphicalOutput(Input* toFollow): Output{toFollow} {
    toFollow->attach(this);
}
void GraphicalOutput::display(Board& board, std::array<bool, 4> settings) {
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
void TextInput::notifyOutputs(Board& board, std::array<bool, 4> settings) {
    for (auto out : outputs) out->display(board, settings);
}
