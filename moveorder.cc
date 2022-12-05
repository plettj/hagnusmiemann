#include "moveorder.h"
#include <algorithm>

/**
 * Static Exchange Evaluation scores for each piece
 */
static constexpr std::array<CentipawnScore, NumPieces> seeScores = {100, 400, 400, 700, 1400, 0};
/**
 *  MVV-LVA values for each piece
 */
static constexpr std::array<HeuristicScore, NumPieces> mvvLvaScores = {0, 3000, 3500, 5000, 10000, 11000};


MoveOrderer::MoveOrderer() {
    moveList.reserve(MaxNumMoves);
}

//haha funny meme number
RandomMoveOrderer::RandomMoveOrderer() : MoveOrderer{}, rng{100000000} {}

void RandomMoveOrderer::seedMoveOrderer(Board& board, bool noisyOnly) {
    this->board = &board;
    this->tacticalSearch = noisyOnly;
    if(noisyOnly) {
        size = board.generateAllNoisyMovesAndChecks(moveList);
    } else {
        size = board.generateAllPseudoLegalMoves(moveList);
    }
}

Move RandomMoveOrderer::pickNextMove(bool noisyOnly) {
    if(size == 0) {
        return Move{};
    }
    std::uniform_int_distribution<> randomDistribution{0, size - 1};
    int random = randomDistribution(rng);
    
    Move move = moveList[random];
    moveList.erase(moveList.begin() + random);
    size--;
    return move;
}

HeuristicMoveOrderer::HeuristicMoveOrderer() : MoveOrderer{}, currentMoveScores{} {
    killerHistoryOne.fill(Move{});
    killerHistoryTwo.fill(Move{});
    for(int i = 0; i < NumColors; ++i) {
        for(int j = 0; j < NumPieces; ++j) {
            for(int k = 0; k < NumSquares; ++k) {
                counterMoves[i][j][k] = Move{};
            }
        }
        quietHistory[i].fill({});
        captureHistory[i].fill({});
    }
}

bool HeuristicMoveOrderer::staticExchangeEvaluation(const Move& move, CentipawnScore margin) {
    assert(board != nullptr);
    //Iteratively go through the pieces that can attack the square in question (taking into account piece values) and 
    //determine who wins.
    CentipawnScore sideBalance = 0;

    //After doing the capture, the next victim is the piece we captured with unless we promote and 
    //that piece changes (done in the below if statement)
    Piece victim = getPieceType(board->getPieceAt(move.getFrom()));

    if(move.getMoveType() != Move::Enpassant) {
        assert(move.getMoveType() != Move::Castle);
        //how valuable is the thing we are capturing
        sideBalance = seeScores[getPieceType(board->getPieceAt(move.getTo()))];
        if(move.getMoveType() == Move::Promotion) {
            victim = move.getPromoType();
            //add the value of promoting our thing to SEE
            sideBalance += seeScores[move.getPromoType()];
            sideBalance -= seeScores[Pawn];
        }
    } else { //enpassant
        sideBalance = seeScores[Pawn];
    }

    sideBalance -= margin; //how far do we have to win SEE for it to be worth it
    
    //If we get a free piece (i.e. no further captures) happen and we still lose,
    //the exchange must just be worse for us
    if(sideBalance < 0) {
        return false;
    }
    sideBalance += seeScores[victim];
    //Even if they get the full piece for free with no further recaptures from us
    //and we still win, we must just win the exchange
    if(sideBalance >= 0) {
        return true;
    }

    //Now that the trivial cases are out of the way, let's roll up our sleeves
    //and count everything.

    //The occupied bitboard given that the first move happened.
    Bitboard occupiedBoard = ((board->sides[White] | board->sides[Black]) ^ (1ull << move.getFrom())) | (1ull << move.getTo());
    if(move.getMoveType() == Move::Enpassant) {
        occupiedBoard ^= (1ull << board->enpassantSquare);
    }

    Bitboard bishops = board->pieces[Bishop] | board->pieces[Queen];
    Bitboard rooks = board->pieces[Rook] | board->pieces[Queen];

    Bitboard allSquareAttackers = (Board::PrecomputedBinary::getBinary().getPawnAttacksFromSquare(move.getTo(), White) & board->sides[Black] & board->pieces[Pawn])
                                | (Board::PrecomputedBinary::getBinary().getPawnAttacksFromSquare(move.getTo(), Black) & board->sides[White] & board->pieces[Pawn])
                                | (Board::PrecomputedBinary::getBinary().getKnightAttacksFromSquare(move.getTo()) & board->pieces[Knight])
                                | (Board::PrecomputedBinary::getBinary().getBishopAttacksFromSquare(move.getTo(), occupiedBoard) & bishops)
                                | (Board::PrecomputedBinary::getBinary().getRookAttacksFromSquare(move.getTo(), occupiedBoard) & rooks)
                                | (Board::PrecomputedBinary::getBinary().getKingAttacksFromSquare(move.getTo()) & board->pieces[King]);
    //so the original piece doesn't attack twice
    allSquareAttackers &= occupiedBoard;

    Color turn = flipColor(board->turn);

    for(Bitboard attackers = allSquareAttackers & board->sides[turn]; attackers != 0;) {
        //find the next piece we can attack with with least value
        if((attackers & board->pieces[Pawn]) != 0) {
            victim = Pawn;
        } else if((attackers & board->pieces[Knight]) != 0) {
            victim = Knight;
        } else if((attackers & board->pieces[Bishop]) != 0) {
            victim = Bishop;
        } else if((attackers & board->pieces[Rook]) != 0) {
            victim = Rook;
        } else if((attackers & board->pieces[Queen]) != 0) {
            victim = Queen;
        } else {
            victim = King;
        }
        //remove this attacker from its current square
        occupiedBoard ^= (1ull << Board::getLsb(attackers & board->pieces[victim]));
        //If we attacked diagonally, we could open up a diagonal
        if(victim == Pawn || victim == Bishop || victim == Queen) {
            allSquareAttackers |= (Board::PrecomputedBinary::getBinary().getBishopAttacksFromSquare(move.getTo(), occupiedBoard) & bishops);
        }
        //if we attacked on a straight, we could open up a rank/file
        if(victim == Rook || victim == Queen) {
            allSquareAttackers |= (Board::PrecomputedBinary::getBinary().getRookAttacksFromSquare(move.getTo(), occupiedBoard) & rooks);
        }
        //Don't add attacks we already figured out
        allSquareAttackers &= occupiedBoard;

        //flip the turn, for which their balance is the inverse of ours (since they are doing the same exchange but sides reverse)
        turn = flipColor(turn);
        //and then add the next victim score
        sideBalance = -sideBalance - 1 - seeScores[victim];

        //if we just are winning even after losing the piece we capture with
        //for nothing, we win the exchange, unless we captured with our king and the opponent still has attackers
        if(sideBalance >= 0) {
            if(victim == King && (allSquareAttackers & board->sides[turn]) != 0) {
                //change who wins as we break out of the loop
                turn = flipColor(turn);
            }
            break;
        }
    }
    //we win if we were not the side who ran out of valid pieces first
    return turn != board->turn;
}

HeuristicScore HeuristicMoveOrderer::getNewHistoryValue(HeuristicScore oldValue, int depth, bool positiveBonus) {
    //Citation: the following formula is one commonly used in the chess engine world,
    //notably by Stockfish, Ethereal, and Weiss
    HeuristicScore bonus = depth > 12 ? 32 : 16 * depth * depth + 128 * std::max(depth - 1, 0);
    HeuristicScore signedBonus = positiveBonus ? bonus : -bonus;
    //The dividing by 16000 is because 16000 is (approximately) the maximum possible value of the history,
    //so this normalizes it
    return oldValue + signedBonus - (oldValue * bonus / 16000);
}

void HeuristicMoveOrderer::updateQuietHeuristics(std::vector<Move>& moveList, int depth) {
    //update killers & counter move

    //The move that caused a beta cut is the final one on the list given to us
    //so it's the new killer/counter move
    Move finalMove = moveList.back();
    if(killerHistoryOne[depth] != finalMove) {
        killerHistoryTwo[depth] = killerHistoryOne[depth];
        killerHistoryOne[depth] = finalMove;
    }
    if(!board->getLastPlayedMove().isMoveNone()) {
        counterMoves[flipColor(board->getTurn())][getPieceType(board->getPieceAt(board->getLastPlayedMove().getTo()))][board->getLastPlayedMove().getTo()] = finalMove;
    }
    //don't record a history heuristic if we didn't calculate anything meaningful
    if(depth == 0 || moveList.size() <= 3) {
        return;
    }
    for(Move& move : moveList) {
        quietHistory[board->getTurn()][move.getFrom()][move.getTo()] = getNewHistoryValue(quietHistory[board->getTurn()][move.getFrom()][move.getTo()], depth, move == finalMove);
    }
}

void HeuristicMoveOrderer::updateNoisyHeuristics(std::vector<Move>& moveList, Move& best, int depth) {
    for(Move& move : moveList) {
        Piece capturedPiece;
        if(move.getMoveType() == Move::Normal) {
            capturedPiece = getPieceType(board->getPieceAt(move.getTo()));
        } else { //enpassant or promotion, consider promotions as pawn captures because we have space in the array there
            capturedPiece = Pawn;
        }
        assert(capturedPiece != King);
        captureHistory[getPieceType(board->getPieceAt(move.getFrom()))][move.getTo()][capturedPiece] = getNewHistoryValue(captureHistory[getPieceType(board->getPieceAt(move.getFrom()))][move.getTo()][capturedPiece], depth, move == best);
    }
}

Move HeuristicMoveOrderer::popBestMove(int beginRange, int endRange) {
    Move best = moveList.front();
    int bestIndex = 0;

    for(int i = beginRange + 1; i < endRange; i++) {
        if(currentMoveScores[moveList[i]] > currentMoveScores[best]) {
            best = moveList[i];
            bestIndex = i;
        }
    }
    moveList.erase(moveList.begin() + bestIndex);
    return best;
}

Move HeuristicMoveOrderer::popFirstMove() {
    Move move = moveList.front();
    moveList.erase(moveList.begin());
    return move;
}

void HeuristicMoveOrderer::setSeeMarginInOrdering(CentipawnScore seeMargin) {
    this->currentSEEMargin = seeMargin;
}

void HeuristicMoveOrderer::seedMoveOrderer(Board& board, bool tacticalSearch) {
    this->board = &board;
    moveList.clear();
    moveList.reserve(MaxNumMoves);
    noisySize = 0;
    quietSize = 0;
    currentStage = GenerateNoisy;
    this->tacticalSearch = tacticalSearch;
    if(tacticalSearch) {
        //Don't play refutation moves here, they're tactical enough such that we
        //should just calculate them and not heuristically refute them.
        killerOne = Move{};
        killerTwo = Move{};
        counter = Move{};
    } else {
        //Generate refutation moves
        killerOne = killerHistoryOne[board.getTotalPlies()];
        killerTwo = killerHistoryTwo[board.getTotalPlies()];
        if(board.getTotalPlies() > 0 && !board.getLastPlayedMove().isMoveNone()) {
            counter = counterMoves[flipColor(board.getTurn())][board.getLastMovedPiece()][board.getLastPlayedMove().getTo()];
        } else {
            counter = Move{};
        }
    }
}

Move HeuristicMoveOrderer::pickNextMove(bool noisyOnly) {
    assert(board != nullptr);
    //If we are doing a tactical search, override the preferences of noisy only
    if(tacticalSearch) {
        noisyOnly = tacticalSearch;
    }
    switch(currentStage) {
        case GenerateNoisy:
            //Step 1. generate noisy moves and then order them.
            //This is always done regardless if we are doing a tactical search or not.
            noisySize = board->generateAllNoisyMoves(moveList);
    
            //set MVV-LVA and history for each noisy move
            for(Move& move : moveList) {
                Piece capturedPiece;
                if(move.getMoveType() == Move::Normal) {
                    capturedPiece = getPieceType(board->getPieceAt(move.getTo()));
                } else { //enpassant or promotion, consider promotions as pawn captures because we have space in the array there
                    capturedPiece = Pawn;
                }
                assert(capturedPiece != King);

                HeuristicScore historyValue = captureHistory[getPieceType(board->getPieceAt(move.getFrom()))][move.getTo()][capturedPiece];
                HeuristicScore mvvLvaValue = mvvLvaScores[capturedPiece] - mvvLvaScores[getPieceType(board->getPieceAt(move.getFrom()))];

                //promoting to queens is a thing that we should prioritize calculating
                if(move.getMoveType() == Move::Promotion && move.getPromoType() == Queen) {
                    historyValue += normalizationConstant;
                }
                currentMoveScores[move] = normalizationConstant + historyValue + mvvLvaValue;
            }
            [[fallthrough]];
        //if there's a good noisy move available, play it first
        case GoodNoisy:
            //set stage if we fell through
            currentStage = GoodNoisy;
            while(noisySize != 0) {
                Move bestMove = popBestMove(0, noisySize);
                noisySize--;

                if(currentMoveScores[bestMove] < 0) {
                    //we have ran out of moves that pass SEE, so we are out of good noisy moves
                    break;
                }

                //if the move doesn't pass SEE, it's a bad capture we should probably not consider
                if(!staticExchangeEvaluation(bestMove, currentSEEMargin)) {
                    currentMoveScores[bestMove] = -161660; //haha funny meme number
                    moveList.emplace_back(bestMove); //place it back at the back of the noisy list to be considered later
                    noisySize++;
                    continue;
                }
                //Ok, our move passed SEE, so let's try it.

                //If we hit a refutation move, then get rid of that refutation
                //move from future consideration since we are about to pick it.
                if(bestMove == killerOne) {
                    killerOne = Move{};
                }
                if(bestMove == killerTwo) {
                    killerTwo = Move{};
                } 
                if(bestMove == counter) {
                    counter = Move{};
                }
                return bestMove;
            }
            [[fallthrough]];
        case KillerOne:
            //We'll now try to play the first killer move. 
            //If we successfully play it, our next move should be the next killer move
            //so set the stage we should be at to do so if we end up returning.
            currentStage = KillerTwo;
            if(!noisyOnly && board->isMovePseudoLegal(killerOne)) {
                return killerOne;
            }
            [[fallthrough]];
        case KillerTwo:
            //We'll now try to play the second killer move. 
            //If we successfully play it, our next move should be the counter move
            //so set the stage we should be at to do so if we end up returning.
            currentStage = Counter;
                
            if(!noisyOnly && board->isMovePseudoLegal(killerTwo)) {
                return killerTwo;
            }
            [[fallthrough]];
        case Counter:
            //Set the stage we should be at if we end up returning
            currentStage = GenerateQuiet;
            if(!noisyOnly && counter != killerOne && counter != killerTwo && board->isMovePseudoLegal(counter)) {
                return counter;
            }
            [[fallthrough]];
        //Step 4:     
        case GenerateQuiet:
            if(!noisyOnly) {
                quietSize = board->generateAllQuietMoves(moveList);
                //set histories
                for(int i = noisySize; i < noisySize + quietSize; ++i) {
                    Move move = moveList[i];
                    //TODO: We could add a followup move or counter move history here
                    currentMoveScores[move] = quietHistory[board->getTurn()][move.getFrom()][move.getTo()];
                }
            }
            [[fallthrough]];    
        case Quiet:
            //set stage if we fell through
            currentStage = Quiet;    
            if(!noisyOnly) {
                while(quietSize != 0) {
                    Move bestMove = popBestMove(noisySize, noisySize + quietSize);
                    quietSize--;

                    if(bestMove == killerOne || bestMove == killerTwo || bestMove == counter) {
                        continue;
                    }
                    return bestMove;
                }
            }
            [[fallthrough]];
        //Step 5. bad noisy. These correspond to captures/noisy moves that are usually obviously ridiculous
        //and should be skipped if we are in a tactical search that only cares about immediately 
        //pressing moves.
        case BadNoisy:
            currentStage = BadNoisy;
            if(!tacticalSearch) {
                while(moveList.size() != 0) {
                    Move move = popFirstMove();
                    if(move == killerOne || move == killerTwo || move == counter) {
                        continue;
                    }
                    return move;
                }
            }
            [[fallthrough]];
        default:
            //Return an empty move, since we have ran out of moves to look for under the given constraints
            return Move{};
    }
}

std::unique_ptr<MoveOrderer> RandomMoveOrderer::clone() const {
    return std::make_unique<RandomMoveOrderer>(*this);
}

std::unique_ptr<MoveOrderer> HeuristicMoveOrderer::clone() const {
    return std::make_unique<HeuristicMoveOrderer>(*this);
}