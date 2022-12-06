#include "fullstrength.h"
#include <algorithm>
#include <cmath>
#include "moveorder.h"

FullStrength::FullStrength(int depthLevel) : DifficultyLevel{EvalLevelFour{}, HeuristicMoveOrderer{}}, depthLevel{depthLevel}, pastScores{} {
    lmrTable[0] = {0};
    for(int depth = 1; depth < LateMoveReductionDepth; ++depth) {
        lmrTable[depth][0] = 0;
        for(int played = 1; played < LateMoveReductionDepth; ++played) {
            //Citation: these constants are used from my old engine badchessengine, from which I got them from Ethereal if I recall correctly
            lmrTable[depth][played] = (int)(0.75 + log(depth) * log(played) / 2.25);
        }
    }
    lmpTable[0][0] = 0;
    lmpTable[1][0] = 0;
    for(int depth = 1; depth < LateMovePruningDepth; ++depth) {
        lmpTable[0][depth] = (int)(2.5 + 2 * depth * depth / 4.5);
        lmpTable[1][depth] = (int)(4 + 4 * depth * depth / 4.5);
    }
}

Move FullStrength::getMove(Board& board) {
    startingMove = board.getTotalPlies();
    //Our difficulty is determined by how far we look, i.e. depth level.
    alphabeta(board, -Infinite, Infinite, depthLevel);
    Move move = bestMoves[board.getBoardHash()];
    assert(!move.isMoveNone());
    return move;
}

CentipawnScore FullStrength::getDeltaPruningMargin(Board& board) {
    CentipawnScore base = board.currentSideAboutToPromote() ? evaluator->getPieceValue(Queen) : evaluator->getPieceValue(Pawn);

    CentipawnScore max = evaluator->getPieceValue(Pawn);
    for(int piece = Pawn; piece <= Queen; piece++) {
        if(board.currentSideHasPiece(static_cast<Piece>(piece))) {
            max = std::max(max, evaluator->getPieceValue(static_cast<Piece>(piece)));
        }
    }
    return base + max;
}

CentipawnScore FullStrength::quiescence(Board& board, CentipawnScore alpha, CentipawnScore beta) {
    //Quiescence is a specialized alpha-beta search focused on tactical moves like captures.
    nodeCount++;
    if(board.isDrawn()) {
        return 0;
    }
    int searchPly = board.getTotalPlies() - startingMove;

    if(searchPly >= MaxDepth) {
        return evaluator->staticEvaluate(board);
    }
    CentipawnScore score = evaluator->staticEvaluate(board);

    //if we beat beta, assume we have a beta cutoff
    if(score >= beta) {
        return score;
    }

    //Delta Pruning: if the best possible outcome of a move can't change our situation,
    //stop calculating
    if(score + getDeltaPruningMargin(board) < alpha) {
        return alpha;
    }
    alpha = std::max(score, alpha);

    MoveOrderer* moveOrderer;
    if(moveOrderers.size() > (size_t)searchPly) {
        moveOrderer = moveOrderers[searchPly].get();
    } else {
        moveOrderers.emplace_back(HeuristicMoveOrderer{}.clone());
        moveOrderer = moveOrderers[searchPly].get();
    }

    moveOrderer->seedMoveOrderer(board, true);
    HeuristicMoveOrderer::setSeeMarginInOrdering(std::max(1, alpha - score - QuiesSeeMargin));
    
    Move move;
    while(!(move = moveOrderer->pickNextMove(true)).isMoveNone()) {
        if(!board.applyMove(move)) {
            continue;
        }
        score = -quiescence(board, -beta, -alpha);
        board.revertMostRecent();

        if(score > alpha) {
            alpha = score;
            if(score >= beta) {
                return beta;
            }
        }
    }
    return alpha;
}

CentipawnScore FullStrength::alphabeta(Board& board, CentipawnScore alpha, CentipawnScore beta, int depth) {
    //if we are in check, look a move farther to ensure we don't miscalculate something after getting out of check
    if(board.isCurrentTurnInCheck()) {
        depth++;
    }

    //if we are out of moves to look at, do a tactical search
    //to ensure we don't hang pieces
    if(depth <= 0 && !board.isCurrentTurnInCheck()) {
        return quiescence(board, alpha, beta);
    }
    int searchPly = board.getTotalPlies() - startingMove;
    bool isRootNode = searchPly == 0;
    bool isPrincipalVariation = alpha != beta - 1;

    nodeCount++;

    //If the board is in a position where we can conclude early (like we have found a forced checkmate already)
    //then do that conclusion. We can't do it in the root node, or else we wouldn't return a bestmove.
    if(!isRootNode) {
        if(board.isDrawn()) {
            return 0;
        }
        if(searchPly >= MaxDepth) {
            return board.isCurrentTurnInCheck() ? 0 : evaluator->staticEvaluate(board);
        }

        //If we have found a forced checkmate, cut off what we search to only try to find a better checkmate
        alpha = std::max(alpha, -Infinite + searchPly);
        beta = std::min(beta, Infinite - searchPly + 1);
        if(alpha >= beta) {
            return alpha;
        } 
    }
    //ensure depth is nonnegative
    depth = std::max(depth, 0);

    CentipawnScore score = -Infinite;
    CentipawnScore bestScore = -Infinite;
    CentipawnScore staticEval = board.isCurrentTurnInCheck() ? NoScore : evaluator->staticEvaluate(board);
    pastScores[searchPly] = staticEval;

    bool hasPositionImproved = !board.isCurrentTurnInCheck() && searchPly >= 2 && staticEval > pastScores[searchPly - 2];
    
    //We're about to do some old school alpha beta search.
    //Alpha represents the lower bound of score a move must have to not be ruled out,
    //beta represents the upper bound of score a move has.
    //Thus, if alpha > beta (i.e. the worst possible score for a move, its lower bound is greater than
    //the best possible score for a move) we can assume that move is simply better and need not calculate
    //further.

    //razoring - if our current static evaluation is significantly lower than alpha,
    //our position sucks and so just ensure we don't miss any tactics then return
    if(!isRootNode && !board.isCurrentTurnInCheck() && !isPrincipalVariation && depth < 2 && staticEval + RazorMargin < alpha) {
        return quiescence(board, alpha, beta);
    }

    //reverse futility, if our position's evaluation is significantly higher than beta
    //then assume it will hold (i.e. our position is so good in every possible way, there's no way we can lose suddenly)
    if(!isRootNode && !board.isCurrentTurnInCheck() && !isPrincipalVariation && depth <= ReverseFutilityDepth && staticEval - ReverseFutilityMargin * depth > beta) {
        return staticEval;
    }

    std::vector<Move> quietsTried;
    std::vector<Move> noisyTried;

    MoveOrderer* moveOrderer;
    if(moveOrderers.size() > (size_t)searchPly) {
        moveOrderer = moveOrderers[searchPly].get();
    } else {
        moveOrderers.emplace_back(HeuristicMoveOrderer{}.clone());
        moveOrderer = moveOrderers[searchPly].get();
    }

    bool noisyOnly = false;
    moveOrderer->seedMoveOrderer(board, false);

    Move move;
    Move bestMove;
    int movesSeen = 0;
    int movesPlayed = 0;
    while(!(move = moveOrderer->pickNextMove(noisyOnly)).isMoveNone()) {
        movesSeen++;

        int improvedIndex = hasPositionImproved ? 1 : 0;
        //Late Move Pruning, if we have calculated many moves in this position already,
        //and we aren't optimistic about this move, skip the quiets
        if(bestScore > -Checkmate && depth <= LateMovePruningDepth && movesSeen >= lmpTable[improvedIndex][depth]) {
            noisyOnly = true;
        }
        bool isMoveTactical = board.isMoveTactical(move);

        HeuristicScore historyHeuristic = isMoveTactical ? HeuristicMoveOrderer::getNoisyHeuristic(board, move) : HeuristicMoveOrderer::getQuietHeuristic(board, move);
        //Quiet Move Pruning. If we prove that a line where we don't lose by force exists in this quiet move,
        //then skip it if its not interesting enough
        if(!isMoveTactical && bestScore > -Checkmate) {
            int lmrDepth = std::max(0, depth - lmrTable[std::min(depth, 63)][std::min(depth, 63)]);
            int futilityMargin = FutilityMargin + lmrDepth * FutilityMarginAdded;

            //futility pruning, if we aren't optimistic about the rest of our quiets then skip them
            if(!board.isCurrentTurnInCheck() && staticEval + futilityMargin + FutilityMarginNoHistory <= alpha && lmrDepth <= FutilityDepth) {
                noisyOnly = true;
            }
        }

        //Static Exchange Evaluation (see moveorder.h for in depth explanation)
        if(bestScore > -Checkmate && depth <= SeeDepth) {
            if(!HeuristicMoveOrderer::staticExchangeEvaluation(board, move, isMoveTactical ? SeeNoisyMargin : SeeQuietMargin)) {
                continue;
            }
        }
        if(!board.applyMove(move)) {
            continue;
        }
        movesPlayed++;
        if(isMoveTactical) {
            noisyTried.emplace_back(move);
        } else {
            quietsTried.emplace_back(move);
        }
        bool doFullSearch = !isPrincipalVariation || movesPlayed > 1;
        //Late Move Reductions.
        //Reduce the search depth after we've explored lots of moves, since
        //under the assumption our ordering heuristics don't suck, these moves are likely to suck
        if(depth > 2 && movesPlayed > 1) {
            int reduction = lmrTable[std::min(depth, 63)][std::min(movesPlayed, 63)];
            
            //if we're not in a principal variation,
            //we are in a generally less important position and can be more aggressive with our reductions
            if(!isPrincipalVariation) {
                reduction++;
            }
            //if our position hasn't been improving, we can generally be certain we won't improve that much
            //and thus can be more aggressive with our reductions
            if(!hasPositionImproved) {
                reduction++;
            }
            //don't reduce as much if we are looking at the refutation moves
            if(!dynamic_cast<HeuristicMoveOrderer&>(*moveOrderer).isAtQuiets()) {
                reduction--;
            }
            //if a move has a really strong history heuristic, don't reduce it as much
            reduction -= std::max(-2, std::min(2, historyHeuristic / 4000));
            //don't reduce into the range of quiescence search
            reduction = std::min(depth - 1, std::max(1, reduction));
            
            //now do the reduced calculation
            //where we force it to be a principal line
            score = -alphabeta(board, -alpha - 1, -alpha, depth - reduction);

            //if we could not beat alpha, do a more minimal search in the future 
            //since it's highly likely we won't be able to beat it without reductions
            //(since heuristically, reduced moves are not likely to beat it)
            doFullSearch = score > alpha && reduction != 1;
        }

        if(doFullSearch) {
            score = -alphabeta(board, -alpha - 1, -alpha, depth - 1);
        }
        //search more fully for for principal variation moves
        if(isPrincipalVariation && (movesPlayed == 1 || score > alpha)) {
            score = -alphabeta(board, -beta, -alpha, depth - 1);
        }
        board.revertMostRecent();

        if(score > bestScore) {
            bestScore = score;
            bestMove = move;

            if(score > alpha) {
                alpha = score;
                bestMoves[board.getBoardHash()] = bestMove;

                //the search failed high, then we can stop looking
                //since our lower bound is better than our upper bound
                //i.e., even the worst move beats the best case scenario 
                if(alpha >= beta) {
                    break;
                }
            }
        }
    }
    //Seed our future heuristics based on the results of this search.
    if(bestScore >= beta) {
        if(!board.isMoveTactical(move)) {
            HeuristicMoveOrderer::updateQuietHeuristics(board, quietsTried, depth);
        }
        HeuristicMoveOrderer::updateNoisyHeuristics(board, noisyTried, bestMove, depth);
    }
    //there were no moves we were able to play, i.e. no legal moves
    if(movesPlayed == 0) {
        //checkmate
        if(board.isCurrentTurnInCheck()) {
            return -Infinite + searchPly;
        //stalemate    
        } else {
            return 0;
        }
    }
    return bestScore;
}