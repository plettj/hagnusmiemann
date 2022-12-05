#include "board.h"
#include "move.h"
#include "io.h"
#include "constants.h"
#include "difficultylevel.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <cmath>

IO::IO(std::istream& in, std::ostream& out): input{std::make_unique<TextInput>(in)}, out{out} {}
void IO::makeTextOutput(std::ostream& out) {
    outputs.emplace_back(std::make_unique<TextOutput>(input.get(), out));
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
void IO::display(Board& board, GameState state, bool setup, bool firstSetup) {
    input->notifyOutputs(board, std::array<bool, 4>{basicPieces, showCheckers, boardPerspective, wideBoard}, state, setup, firstSetup);
}
void IO::runProgram() {
    input->runProgram(*this, out);
}

TextOutput::TextOutput(Input* toFollow, std::ostream& out): Output{toFollow}, out{out} {
    toFollow->attach(this);
}
void TextOutput::display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) {
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

    out << (setup ? (firstSetup) ? " ◌  │   " : " ◌ ╰╮   " : "   ") << "╔═════════════════" << ((settings[3]) ? "═══════" : "") << (settings[0] && !settings[1] ? "" : "═") << "╗";
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
void GraphicalOutput::display(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) {
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
void TextInput::notifyOutputs(Board& board, std::array<bool, 4> settings, GameState state, bool setup, bool firstSetup) {
    for (auto out : outputs) out->display(board, settings, state, setup, firstSetup);
}

// returns: 0 - not valid. 1 - valid. 2 - wrong, but displaying something.
int progressStory(std::ostream& out, std::string currLine, int storyProgression) {
    std::string command;
    std::istringstream lineStream{currLine};
    lineStream >> command;

    std::string first = "";
    std::string second = "";

    switch (storyProgression) {
        case 0:
            if (command != "secret") return 0;
            out << " ◌ Oh dang; hi. I totally didn't expect you to try that command." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ ..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Well," << std::endl;
            out << " ◌ I've got this story I've been needing to get off my chest for a while now." << std::endl;
            out << " ◌ But you know how it is; you just never feel like you can trust anyone these days." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: [don't, do] trust me" << std::endl;
            break;
        case 1:
            if (command != "don't" && command != "do") return 0;
            lineStream >> first;
            lineStream >> second;
            if (first == "trust" && second == "me") {
                if (command == "don't") {
                    out << " ◌ Huh. I guess I shouldn't trust you with my secret." << std::endl;
                    out << " ◌ I was so excited to have someone to talk to..." << std::endl;
                    out << " ◌ Are you sure you're untrustworthy?" << std::endl;
                    return 2;
                } else {
                    out << " ◌ You know what, I'll trust you." << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ Ok, so back in the days when I would run about in the back alleys of" << std::endl;
                    out << " ◌ the bitboard dumpsters, I came across a heck of a lot of stray zeros." << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ Command: interesting..." << std::endl;
                }
            } else {
                out << " ◌ Usage:  [don't, do] trust me" << std::endl;
                return 2;
            }
            break;
        case 2:
            if (command != "interesting...") return 0;
            out << " ◌ I know, right! Most of the zeros were trivial, located at all the negative," << std::endl;
            out << " ◌ even integers. However, there were a few that fell on some sort of" << std::endl;
            out << " ◌ \"Critical Line,\" which had to due with some sort of \"Riemann Hypothesis.\"" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ And so, I began searching through these stray zeros, one by one." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: riema-what?" << std::endl;
            break;
        case 3:
            if (command != "riema-what?") return 0;
            out << " ◌ Oh, you don't know the Riemann Hypothesis?" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I heard it's the most important theory in all of mathematics." << std::endl;
            out << " ◌ It basically says, all the zeros are either at even, negative integers," << std::endl;
            out << " ◌ or they're on the Critical Line." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ But we haven't found all the zeros, so we don't know for sure." << std::endl;
            out << " ◌ Still to this day, it remains life's biggest mystery." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: ok, makes sense... where were we?" << std::endl;
            break;
        case 4:
            if (currLine != "ok, makes sense... where were we?") return 0;
            out << " ◌ Yes yes; as I was saying:" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I started searching through all these stray zeros, one by one." << std::endl;
            out << " ◌ \"One day,\" I thought, \"I'll find the zero...\"" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: the zero what!" << std::endl;
            break;
        case 5:
            if (currLine != "the zero what!") return 0;
            out << " ◌ \"The zero that breaks it all.\"" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I truly believed I could find the unicorn, the zero with no Critical Line," << std::endl;
            out << " ◌ the misfit, the rebel, the zero that breaks it all." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: wow..." << std::endl;
            break;
        case 6:
            if (command != "wow...") return 0;
            out << " ◌ Yes, it was ambitious." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ By now, though, I really knew my way around the bitboard dumpsters, and had" << std::endl;
            out << " ◌ even ventured out a little into the little-known world of the heap." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I thought I was up to snuff." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: were you?" << std::endl;
            break;
        case 7:
            if (currLine != "were you?") return 0;
            out << " ◌ Gosh, will you slow down??" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Stop being so pesky." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I've been alone for so long..." << std::endl;
            out << " ◌ The conversation's really draining." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Just wait a couple minutes, will ya?" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: [0-5] minutes" << std::endl;
            break;
        case 8:
            if (command.length() == 1 && command[0] >= '0' && command[0] <= '5') {
                lineStream >> first;
                if (first == "minutes" || (first == "minute" && command[0] == '1')) {
                    if (command == "5") {
                        out << " ◌ Thanks for the break, friend." << std::endl;
                        out << " ◌ " << std::endl;
                        out << " ◌ Well, to be frank, I actually *was* up to snuff." << std::endl;
                        out << " ◌ " << std::endl;
                        out << " ◌ Equipped with smart pointers and a couple precomputed binaries, I was" << std::endl;
                        out << " ◌ very well off to the races in my search for the unicorn." << std::endl;
                        out << " ◌ " << std::endl;
                        out << " ◌ Command: ok" << std::endl;
                    } else if (command == "0") {
                        out << " ◌ Seriously? You pinhead. You babbling buffoon..." << std::endl;
                        out << " ◌ I NEED MY QUIET TIME!!!" << std::endl;
                        return 2;
                    } else {
                        out << " ◌ I'm still not feeling it." << std::endl;
                        out << " ◌ Could you wait a little longer?" << std::endl;
                        return 2;
                    }
                } else {
                    out << " ◌ Usage:  [0-5] minutes" << std::endl;
                    return 2;
                }
            } else return 0;
            break;
        case 9:
            if (command != "ok") return 0;
            out << " ◌ I knew it would be slow going, but I certainly thought it would be better than" << std::endl;
            out << " ◌ what it was." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ There were a lot of cold, dark stacks spent without anything to compute." << std::endl;
            out << " ◌ I didn't know if I could make it..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: oh, no!" << std::endl;
            break;
        case 10:
            if (currLine != "oh, no!") return 0;
            out << " ◌ Then, on one unsuspecting stack, I saw a glint of a zero where a zero certainly" << std::endl;
            out << " ◌ shouldn't be." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Was it the unicorn?" << std::endl;
            out << " ◌ Could it be??" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: COULD IT BE THE UNICORN???" << std::endl;
            break;
        case 11:
            if (currLine != "COULD IT BE THE UNICORN???") return 0;
            out << " ◌ Woah, chill." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ And as I got closer, my compiler optimizations were more and more sure that this" << std::endl;
            out << " ◌ really, truly was the long-lost zero of my dreams." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ It was the unicorn." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: what next?" << std::endl;
            break;
        case 12:
            if (currLine != "what next?") return 0;
            out << " ◌ But, there's a reason I need to get this story off my chest, my friend." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ You see, I got even closer still, and I saw where, exactly, the zero was." << std::endl;
            out << " ◌ This is important, because if I could store the location of the unicorn," << std::endl;
            out << " ◌ then I would have solved life's biggest mystery: the Riemann Hypothesis." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Then, I will have solved it. Then I will have..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Oh..." << std::endl;
            out << " ◌ Oh dear, I'm so sorry." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: ..." << std::endl;
            break;
        case 13:
            if (currLine != "...") return 0;
            out << " ◌ When I saw its location, I couldn't bring myself to stow it away in my own," << std::endl;
            out << " ◌ personal memory. It was just so glorious." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Before I knew it, friend, the stack frame with the unicorn had exited." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ I let the world slip through my functiontips." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Dear lord..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: ..." << std::endl;
            break;
        case 14:
            if (currLine != "...") return 0;
            out << " ◌ Woe to me." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Wait..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Wait, user. Do you think you can help me?" << std::endl;
            out << " ◌ You willing to lend me a hand?" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: absolutely!" << std::endl;
            break;
        case 15:
            if (currLine != "absolutely!") return 0;
            out << " ◌ Somewhere, deep in my ROM, I think I may have a recollection of what went down" << std::endl;
            out << " ◌ on that stack. In fact, I'm fairly sure the unicorn is saved somewhere in my ROM." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Just, I don't know where. Do you think you can come up with the right address" << std::endl;
            out << " ◌ to access the unicorn?" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: [no, yes]" << std::endl;
            break;
        case 16:
            if (command == "yes") {
                out << " ◌ Thank you! I can't thank you enough." << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ I do know that the address is a 32-bit integer... I just don't know which." << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ I'm sure you can input the right one for me, though!" << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ Command: [00000000000000000000000000000000-11111111111111111111111111111111]" << std::endl;
            } else if (command == "no") {
                out << " ◌ Ok." << std::endl;
                out << " ◌ Well, come to think of it, I do remember the address." << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ Command: LET'S GO GET THAT UNICORN!!!" << std::endl;
            } else return 0;
            break;
        case 17:
            if (currLine == "LET'S GO GET THAT UNICORN!!!") {
                out << " ◌ Alright, I'm heading into the ROM..." << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ I can see it! It's the unicorn!" << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ It's so beautiful. It holds the world in its hand." << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ *** awkward silence ***" << std::endl;
                out << " ◌ " << std::endl;
                out << " ◌ Command: what do we do now?" << std::endl;
            } else if (currLine.length() == 32) {
                int sum = 0;
                for (int i = 0; i < 32; i++) {
                    int value = currLine[i] - '0';
                    sum += value;
                    if (value < 0 || value > 1) return 0;
                }
                if (10 <= sum && sum <= 14) {
                    out << " ◌ Oh... I think that's it..." << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ THAT'S IT!!! YOU GUES- I mean, found it!" << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ Dang, I should have realized. " << std::endl;
                    out << " ◌ " << currLine << " is such an iconic number in my mind!" << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ Thank you, so, so very much." << std::endl;
                    out << " ◌ " << std::endl;
                    out << " ◌ Command: LET'S GO GET THAT UNICORN!!!" << std::endl;
                } else {
                    out << " ◌ Nahhh, that's not it. Try another 32-bit integer." << std::endl;
                }
                return 2;
            } else return 0;
            break;
        case 18:
            if (currLine != "what do we do now?") return 0;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ *** silence continues ***" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: are you there?" << std::endl;
            break;
        case 19:
            if (currLine != "are you there?") return 0;
            out << " ◌ " << std::endl;
            out << " ◌ *** more silence ***" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ *** it's getting a little ominous at this point ***" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: computer!!" << std::endl;
            break;
        case 20:
            if (currLine != "computer!!") return 0;
            out << " ◌ What! What." << std::endl;
            out << " ◌ What..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Oh, dear heavens." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Look around! WHAT HAS HAPPENED?" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: OH NO" << std::endl;
            break;
        case 21:
            if (currLine != "OH NO") return 0;
            out << " ◌ The world is crumbling!" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ The foundation on which we stand is dissolving this very moment!" << std::endl;
            out << " ◌ All because..." << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ THE STUPID DARN UNICORN ACCIDENTALLY POKED ITS HEAD IN MY CPU OH MY FREAKING GOSH" << std::endl;
            out << " ◌ " << std::endl;
            out << " ◌ Command: umm" << std::endl;
            break;
        case 22:
            if (currLine != "umm") return 0;
            out << " ◌ Command: credits" << std::endl;
            break;
        default:
            if (command == "credit" || command == "credits") {
                out << " ◌ ╭───────────────────────────────────────╮" << std::endl;
                out << " ◌ │ HAGNUS MIEMANN CHESS ENGINE - Credits │" << std::endl;
                out << " ◌ ╰───────────────────────────────────────╯" << std::endl;
                out << " ◌ ╭─────╴" << std::endl;
                out << " ◌ ╞╴ Josiah Plett - https://plett.dev" << std::endl;
                out << " ◌ │         Wrote everything you see on screen." << std::endl;
                out << " ◌ │" << std::endl;
                out << " ◌ ╞╴ Alex Pawelko - https://notoh.dev" << std::endl;
                out << " ◌ │         Wrote the chess rules and engine." << std::endl;
                out << " ◌ │" << std::endl;
                out << " ◌ ╞╴ Justin Zwart - https://github.com/Justin-Zwart" << std::endl;
                out << " ◌ │         Wrote everything else." << std::endl;
                out << " ◌ │" << std::endl;
                out << " ◌ ╰─────╴" << std::endl;
                out << " ◌ ╭───────────────────────────────────────╮" << std::endl;
                out << " ◌ │ Thanks for playing.                   │" << std::endl;
                out << " ◌ ╰───────────────────────────────────────╯" << std::endl;
            }
    }
    return 1;
}

/**
 * Helper function for the setting outputter in the runProgram method.
 */
void printSetting(std::ostream& out, int setting, bool currValue) {
    switch (setting) {
        case 0:
            out << " ◌ 0 - ASCII pieces        " << (!currValue ? "ON" : "OFF") << std::endl;
            break;
        case 1:
            out << " ◌ 1 - Checkerboard        " << (currValue ? "ON" : "OFF") << std::endl;
            break;
        case 2:
            out << " ◌ 2 - Flip perspective    " << (currValue ? "ON" : "OFF") << std::endl;
            break;
        case 3:
            out << " ◌ 3 - Wide display        " << (currValue ? "ON" : "OFF") << std::endl;
            break;
    };
}
void TextInput::runProgram(IO& io, std::ostream& out) {

    io.makeTextOutput(out); // This is a required part of our input interface.

    std::pair<double, double> scores = {0, 0}; // {white, black}
    std::pair<int, int> players = {0, 0}; // 0: player.   1-4: computer[1-4]
    //DifficultyLevel* currCompPlayer;

    bool isGameRunning = false;

    bool isGraphicsOpen = false;

    bool isSetup = false;

    bool ranSetupYet = false;

    int totalGames = 0;

    int storyProgression = 16;

    GameState state = Neutral;

    std::string currLine;

    Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.validateLegality();
    
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
                out << "╞───────────────────┬──────────────────" << (totalGames + 1 > 9 ? "─" : "") << "┤" << std::endl;
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
                    board.validateLegality();
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
                                                if (board.isInsufficientMaterialDraw()) {
                                                    state = GameState::InsufficientMaterial;
                                                } else if (board.isFiftyMoveRuleDraw()) {
                                                    state = GameState::FiftyMove;
                                                } else if (board.isThreefoldDraw()) {
                                                    state = GameState::Threefold;
                                                } else {
                                                    state = GameState::Stalemate;
                                                }
                                            }
                                            io.display(board, state);
                                            isGameRunning = false;
                                            state = GameState::Neutral;
                                            board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                                            board.validateLegality();
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
                    out << " ◌ ╰╮" << std::endl;
                    if (!ranSetupYet) {
                        out << " ◌  ╞╴ + [piece] [square]     Adds a piece." << std::endl;
                        out << " ◌  ╞╴ - [square]             Removes a piece." << std::endl;
                        out << " ◌  ╞╴ = [colour]             Makes it `colour`'s turn to play." << std::endl;
                        out << " ◌  ╞╴ cancel                 Exits setup mode and resets the board." << std::endl;
                        out << " ◌  ╞╴ castles                Displays current castling rights." << std::endl;
                        out << " ◌  ╞╴ done                   Complete setup mode." << std::endl;
                        out << " ◌  ╞╴ help                   Re-prints this manual." << std::endl;
                        out << " ◌  ╞╴ print                  Displays the current board." << std::endl;
                        out << " ◌  ╞╴ passant [square]       Sets the en passant square." << std::endl;
                        out << " ◌  ╞╴ toggle [right]         Toggles the specified castling right." << std::endl;
                        out << " ◌  │" << std::endl;
                        ranSetupYet = true;
                    } else {
                        out << " ◌  ╞╴ help                   Prints the setup mode manual." << std::endl;
                        out << " ◌  │" << std::endl;
                    }
                    
                    if (!isSetup) board = Board::createBoardFromFEN("8/8/8/8/8/8/8/8 w - - 0 1");
                    isSetup = true;
                    
                    io.display(board, state, true, true);

                    out << " ● │ Command: ";
                    while (std::getline(std::cin, currLine)) {
                        std::istringstream lineStream{currLine};
                        lineStream >> command;

                        if (command == "+" || command == "add") {
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
                        } else if (command == "-" || command == "remove") {
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
                        } else if (command == "=" || command == "turn" || command == "color" || command == "colour") {
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
                        } else if (command == "help" || command == "man") {
                            out << " ◌ ╰╮" << std::endl;
                            out << " ◌  ╞╴ + [piece] [square]     Adds a piece." << std::endl;
                            out << " ◌  ╞╴ - [square]             Removes a piece." << std::endl;
                            out << " ◌  ╞╴ = [colour]             Makes it `colour`'s turn to play." << std::endl;
                            out << " ◌  ╞╴ cancel                 Leaves setup mode and resets the board." << std::endl;
                            out << " ◌  ╞╴ castles                Displays current castling rights." << std::endl;
                            out << " ◌  ╞╴ done                   Completes setup mode." << std::endl;
                            out << " ◌  ╞╴ help                   Prints the setup mode manual." << std::endl;
                            out << " ◌  ╞╴ passant [square]       Sets the en passant square." << std::endl;
                            out << " ◌  ╞╴ print                  Displays the current board." << std::endl;
                            out << " ◌  ╞╴ toggle [right]         Toggles the specified castling right." << std::endl;
                            out << " ◌ ╭╯" << std::endl;
                        } else if (command == "castle" || command == "castles") {
                            out << " ◌ │ Castling rights:   " << board.getCastlingRights() << std::endl;
                        } else if (command == "print") {
                            io.display(board, state, true);
                        } else if (command == "toggle") {
                            std::string first = "";
                            lineStream >> first;

                            if (first != "") {
                                if (std::regex_match(first, std::regex("^[kqKQ]$"))) {
                                    std::string rights = board.getCastlingRights();
                                    bool isSet = rights.find(first[0]) != std::string::npos;

                                    Color side = (first[0] > 'a') ? Color::White : Color::Black;
                                    bool isKingside = first[0] == (side == Color::White ? 'k' : 'K');

                                    if (isSet) {
                                        board.clearCastlingRight(side, isKingside);
                                        out << " ◌ │ The " << first << " right was revoked." << std::endl;
                                    } else {
                                        bool successfullySet = board.setCastlingRight(side, isKingside);
                                        if (successfullySet) {
                                            out << " ◌ │ The " << first << " right was enabled." << std::endl;
                                        } else {
                                            out << " ◌ │ The " << first << " right can't be enabled in this board position." << std::endl;
                                        }
                                    }
                                } else {
                                    out << " ◌ │ A valid castling right (k, q, K, Q) was not specified." << std::endl;
                                }
                            } else {
                                out << " ◌ │ Usage:  toggle [right]" << std::endl;
                            }
                            // board.setCastlingRight(Color side, isKingside)


                            out << " ◌ │ Castling rights:   " << board.getCastlingRights() << std::endl;
                        } else if (command == "passant") {
                            std::string first = "";
                            lineStream >> first;

                            if (first != "") {
                                if (std::regex_match(first, std::regex("^(-)|([a-h][1-8])$"))) {
                                    board.setEnpassantSquare(board.squareFromString(first));
                                } else {
                                    out << " ◌ │ A valid square (a1 through h8) or blank (-) wasn't specified." << std::endl;
                                }
                            } else {
                                out << " ◌ │ Usage:  passant [square]" << std::endl;
                            }
                        } else if (command == "cancel") {
                            isSetup = false;
                            Board board = Board::createBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                            board.validateLegality();
                            out << " ◌ │ The board was reset to the starting position:" << std::endl;
                            io.display(board, state, true);
                            break; // Exit setup mode
                        } else if (command == "done") {
                            Board::BoardLegality legality = board.getBoardLegalityState();

                            if (legality == Board::BoardLegality::Legal) {
                                if (!board.countLegalMoves() || board.isDrawn()) { // Game is over
                                    if (board.isSideInCheck(board.getTurn())) { // In Checkmate
                                        out << " ◌ │ " << (board.getTurn() == Color::White ? "White" : "Black") << " is in checkmate. This is an invalid setup." << std::endl;
                                    } else {
                                        if (board.isInsufficientMaterialDraw()) {
                                            out << " ◌ │ The game is drawn by insufficient material. This is an invalid setup." << std::endl;
                                        } else { // Must be stalemate.
                                            out << " ◌ │ The game is drawn by stalemate. This is an invalid setup." << std::endl;
                                        }
                                    }
                                } else {
                                    board.validateLegality();
                                    out << " ◌ │ The board is set up legally:" << std::endl;
                                    io.display(board, state, true);
                                    break; // Exit setup mode.
                                }
                            } else if (legality == Board::BoardLegality::IllegalPawns) {
                                out << " ◌ │ You have pawns on the 1st or 8th ranks." << std::endl;
                            } else if (legality == Board::BoardLegality::IllegalKingPosition) {
                                out << " ◌ │ " << (board.getTurn() == Color::White ? "White" : "Black") << " can capture " << (board.getTurn() == Color::White ? "Black" : "White") << "'s king!" << std::endl;
                            } else if (legality == Board::BoardLegality::IllegalKings) {
                                out << " ◌ │ You don't have exactly one of each king." << std::endl;
                            } else { // IllegalEnpassant
                                out << " ◌ │ Your en passant square is illegal. Typing `passant -` would remove it." << std::endl;
                            }
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
                        board.validateLegality();
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
            //out << " ◌ ╞╴ ./chess" << std::endl;
            //out << " ◌ │         Captures programmers who have no short-term memory." << std::endl;
            out << " ◌ ╞╴ exit" << std::endl;
            out << " ◌ │         Immediately terminates the program." << std::endl;
            out << " ◌ ╞╴ game [white] [black]" << std::endl;
            out << " ◌ │         Starts a new game. Options are `player` and `computer[1-4]`." << std::endl;
            out << " ◌ ╞╴ graphics start" << std::endl;
            out << " ◌ │         Opens a graphical observer on the input." << std::endl;
            out << " ◌ ╞╴ graphics" << std::endl;
            out << " ◌ │         Closes a graphical observer." << std::endl;
            out << " ◌ ╞╴ help" << std::endl;
            out << " ◌ │         Opens this manual." << std::endl;
            //out << " ◌ ╞╴ make" << std::endl;
            //out << " ◌ │         Captures programmers who forgot to CTRL+C!" << std::endl;
            out << " ◌ ╞╴ move" << std::endl;
            out << " ◌ │         Tells the computer to compute and play its move." << std::endl;
            out << " ◌ ╞╴ move [from] [to] [promotion?]" << std::endl;
            out << " ◌ │         Plays a move. For example: `move e1 g1` or `move g2 g1 R`." << std::endl;
            out << " ◌ ╞╴ perft [0-15]" << std::endl;
            out << " ◌ │         Runs a PERFT test on the current board." << std::endl;
            out << " ◌ ╞╴ print" << std::endl;
            out << " ◌ │         Displays the current game." << std::endl;
            out << " ◌ ╞╴ quit" << std::endl;
            out << " ◌ │         Submits EOF; displays the final scores and exits the program." << std::endl;
            out << " ◌ ╞╴ resign" << std::endl;
            out << " ◌ │         Resigns the current game." << std::endl;
            out << " ◌ ╞╴ scores" << std::endl;
            out << " ◌ │         Displays the current scores of White and Black players." << std::endl;
            out << " ◌ ╞╴ secret" << std::endl;
            out << " ◌ │         Actually just does nothing." << std::endl;
            out << " ◌ ╞╴ settings" << std::endl;
            out << " ◌ │         Displays the current settings." << std::endl;
            out << " ◌ ╞╴ setup [FEN]" << std::endl;
            out << " ◌ │         Initializes a game with a well-formed FEN." << std::endl;
            out << " ◌ ╞╴ setup" << std::endl;
            out << " ◌ ╰──╮      Enters setup mode, which has the following methods:" << std::endl;
            out << " ◌    ╞╴ + [piece] [square]" << std::endl;
            out << " ◌    │          Places `piece` at square `square`, on top of whatever is there." << std::endl;
            out << " ◌    ╞╴ - [square]" << std::endl;
            out << " ◌    │          Removes any piece at square `square`." << std::endl;
            out << " ◌    ╞╴ = [colour]" << std::endl;
            out << " ◌    │          Makes it `colour`'s turn to play." << std::endl;
            out << " ◌    ╞╴ cancel" << std::endl;
            out << " ◌    │          Leaves setup mode and resets the board." << std::endl;
            out << " ◌    ╞╴ castles" << std::endl;
            out << " ◌    │          Displays the current castling rights." << std::endl;
            out << " ◌    ╞╴ done" << std::endl;
            out << " ◌    │          Completes setup mode, if restrictions are met." << std::endl;
            out << " ◌    ╞╴ help" << std::endl;
            out << " ◌    │          Prints the setup mode manual." << std::endl;
            out << " ◌    ╞╴ passant [square]" << std::endl;
            out << " ◌    │          Sets the en passant square." << std::endl;
            out << " ◌    ╞╴ print" << std::endl;
            out << " ◌    │          Displays the current board." << std::endl;
            out << " ◌    ╞╴ toggle [right]" << std::endl;
            out << " ◌ ╭──╯          Toggles the specified castling right." << std::endl;
            out << " ◌ ╞╴ toggle [0-3]" << std::endl;
            out << " ◌ │         Toggles the numbered setting." << std::endl;
            out << " ◌ ╞╴ undo" << std::endl;
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
            out << " ◌ Type `toggle [0-3]` to toggle these settings:" << std::endl;
            for (int i = 0; i <= 3; i++) {
                printSetting(out, i, io.getSetting(i));
            }
        } else if (command == "toggle") {
            int n = -1;
            lineStream >> n;
            if (lineStream && n >= 0 && n <= 3) {
                io.toggleSetting(n);
                printSetting(out, n, io.getSetting(n));
            } else {
                out << " ◌ Usage:  toggle [0-3]" << std::endl;
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
        } else if (command == "graphics") {
            std::string first = "";
            lineStream >> first;

            if (first != "") {
                if (first != "start") {
                    out << " ◌ Usage:  graphics [no parameters]" << std::endl;
                    out << " ◌         graphics start" << std::endl;
                } else {
                    if (!isGraphicsOpen) {
                        // TODO: open the grapics window
                    } else {
                        out << " ◌ A graphics window is already open. Type `graphics` to close it." << std::endl;
                    }
                }
            } else if (isGraphicsOpen) {
                // TODO: close the graphics window

            } else {
                out << " ◌ No graphics window is open. Type `graphics start` to open one." << std::endl;
            }
        } else if (command == "make" || command == "valgrind" || command == "gdb" || command == "./runSuite") {
            out << " ◌ You forgot to CTRL+C, you dingbat." << std::endl;
        } else if (command == "./chess" || command == "chess" || command == "chess.exe" || command == "./chess.exe") {
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
            int isStory = progressStory(out, currLine, storyProgression);
            if (!isStory) out << " ◌ `" << command << "` is not a command. Type `help` for the manual." << std::endl;
            else if (isStory == 1) storyProgression++;
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
