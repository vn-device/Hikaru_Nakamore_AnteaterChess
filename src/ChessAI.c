/******************************************************************************
 * File: ChessAI.c
 * Author: Jonghyun Choi, Vinh Nguyen
 * Date: April 14, 2026
 *
 * Description:
 * Implements the core strategy logic for the computer player module.
 * Handles the evaluation of board positions, material advantage calculations,
 * and the execution of the Minimax algorithm with Alpha-Beta pruning to 
 * generate competitive moves for the PvE game modes across all difficulties.
 *****************************************************************************/

#include <stdlib.h>
#include <time.h>
#include "ChessAI.h"
#include "MoveValidation.h"

//=============================================================================

/*
 * EvaluateBoard
 *
 * Description:
 * Calculates the static material advantage for the AI's color on the current board.
 * A positive score indicates the AI is winning; a negative score indicates the 
 * human opponent is winning. It uses the centipawn piece values defined in ChessAI.h.
 *
 * Parameters:
 * pBoard  - Pointer to the active game board state to evaluate.
 * aiColor - The piece color controlled by the AI ('w' or 'b').
 *
 * Returns:
 * int - The computed material score.
 */
int EvaluateBoard(Board* pBoard, char aiColor)
{
    int score = 0;
    char humanColor = (aiColor == 'w') ? 'b' : 'w';

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            Piece p = pBoard->grid[r][c];
            if (p.type == ' ') continue;

            int pieceValue = 0;
            switch (p.type) {
                case 'P': pieceValue = VAL_PAWN;     break;
                case 'N': pieceValue = VAL_KNIGHT;   break;
                case 'B': pieceValue = VAL_BISHOP;   break;
                case 'A': pieceValue = VAL_ANTEATER; break;
                case 'R': pieceValue = VAL_ROOK;     break;
                case 'Q': pieceValue = VAL_QUEEN;    break;
                case 'K': pieceValue = VAL_KING;     break;
            }

            if (p.color == aiColor) {
                score += pieceValue;
            }
            else if (p.color == humanColor) {
                score -= pieceValue;
            }
        }
    }

    return score;
}

//=============================================================================

/*
 * Minimax
 *
 * Description:
 * A static helper function that executes a recursive Minimax search tree with 
 * Alpha-Beta Pruning. It explores all legal move combinations up to the specified 
 * depth to find the optimal guaranteed material outcome, pruning branches that 
 * the opponent will optimally avoid.
 *
 * Parameters:
 * pBoard       - Pointer to the current game board state (usually a cloned temp board).
 * depth        - The remaining number of plies (half-turns) to search.
 * alpha        - The minimum score the maximizing player is guaranteed.
 * beta         - The maximum score the minimizing player is guaranteed.
 * isMaximizing - Flag indicating if the current ply is the AI's turn (1) or human's (0).
 * aiColor      - The piece color controlled by the AI ('w' or 'b').
 *
 * Returns:
 * int - The best evaluated material score for the current search branch.
 */
static int Minimax(Board* pBoard, int depth, int alpha, int beta, int isMaximizing, char aiColor)
{
    if (depth == 0) {
        return EvaluateBoard(pBoard, aiColor);
    }

    char currentColor = isMaximizing ? aiColor : (aiColor == 'w' ? 'b' : 'w');  
    MoveList legalMoves;
    GenerateLegalMoves(pBoard, currentColor, &legalMoves);

    /* Terminal state failsafe: Checkmate or Stalemate */
    if (legalMoves.count == 0) {
        return EvaluateBoard(pBoard, aiColor);
    }

    if (isMaximizing) {
        int maxEval = -1000000;
        for (int i = 0; i < legalMoves.count; i++) {
            Board temp = *pBoard;
            ApplyMove(&temp, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, 
                             legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
            
            int eval = Minimax(&temp, depth - 1, alpha, beta, 0, aiColor);
            
            if (eval > maxEval) maxEval = eval;
            if (eval > alpha) alpha = eval;
            
            /* Prune: The minimizing opponent already has a better path higher up the tree */
            if (beta <= alpha) break; 
        }
        return maxEval;
    } 
    else {
        int minEval = 1000000;
        for (int i = 0; i < legalMoves.count; i++) {
            Board temp = *pBoard;
            ApplyMove(&temp, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, 
                             legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
            
            int eval = Minimax(&temp, depth - 1, alpha, beta, 1, aiColor);
            
            if (eval < minEval) minEval = eval;
            if (eval < beta) beta = eval;
            
            /* Prune: The maximizing opponent already has a better path higher up the tree */
            if (beta <= alpha) break; 
        }
        return minEval;
    }
}

/*
 * DetermineAIMove
 *
 * Description:
 * Evaluates the current board state and computes the optimal move for the AI
 * based on the selected difficulty level constraints:
 * - Easy ('1'): Utilizes a Depth-1 greedy search to prioritize immediate captures.
 * - Medium ('2'): Utilizes a Depth-3 Minimax search with Alpha-Beta pruning.
 * - Hard ('3'): Utilizes a Depth-5 Minimax search with Alpha-Beta pruning.
 * * To prevent deterministic gameplay loops, it collects all moves that share the 
 * optimal score and uses a randomized selection among them.
 *
 * Parameters:
 * pBoard     - Pointer to the active game board state.
 * aiColor    - The piece color controlled by the AI ('w' or 'b').
 * difficulty - The configured bot difficulty level ('1'=Easy, '2'=Medium, '3'=Hard).
 *
 * Returns:
 * Move - A struct containing the calculated optimal source and destination coordinates.
 */
Move DetermineAIMove(Board* pBoard, char aiColor, char difficulty)
{
    MoveList legalMoves;
    GenerateLegalMoves(pBoard, aiColor, &legalMoves);

    if (legalMoves.count == 0) {
        Move nullMove = {0, 0, 0, 0};
        return nullMove;
    }

    /* Difficulty 1: Easy - Depth-1 Greedy Search */
    if (difficulty == '1') {
        int bestScore = -1000000;
        MoveList candidateMoves;
        InitMoveList(&candidateMoves);

        for (int i = 0; i < legalMoves.count; i++) {
            Board temp = *pBoard;
            ApplyMove(&temp, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
            int score = EvaluateBoard(&temp, aiColor);
            
            if (score > bestScore) {
                bestScore = score;
                InitMoveList(&candidateMoves);
                AddMove(&candidateMoves, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
            } 
            /* Append moves that result in the same board evaluation */
            else if (score == bestScore) {
                AddMove(&candidateMoves, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
            }
        }
        
        srand(time(NULL));
        return candidateMoves.moves[rand() % candidateMoves.count];
    }

    /* Difficulty 2 & 3: Alpha-Beta Pruning Execution
     * Medium evaluates 3 total plies (1 root + 2 recursive).
     * Hard evaluates 5 total plies (1 root + 4 recursive). 
     */
    int recursiveDepth = (difficulty == '2') ? 2 : 4;
    int bestScore = -1000000;
    int alpha = -1000000;
    int beta = 1000000;
    
    MoveList candidateMoves;
    InitMoveList(&candidateMoves);

    for (int i = 0; i < legalMoves.count; i++) {
        Board temp = *pBoard;
        ApplyMove(&temp, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
        
        /* * The root node acts as the initial maximizer step. 
         * The first recursive call hands control to the minimizer (isMaximizing = 0).
         */
        int score = Minimax(&temp, recursiveDepth, alpha, beta, 0, aiColor);
        
        if (score > bestScore) {
            bestScore = score;
            InitMoveList(&candidateMoves); // Flush the current movelist
            AddMove(&candidateMoves, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
        }
        else if (score == bestScore) {
            AddMove(&candidateMoves, legalMoves.moves[i].fRow, legalMoves.moves[i].fCol, legalMoves.moves[i].tRow, legalMoves.moves[i].tCol);
        }
        
        /* Update the root alpha bound for subsequent sibling nodes */
        if (bestScore > alpha) {
            alpha = bestScore;
        }
    }
    
    srand(time(NULL));
    return candidateMoves.moves[rand() % candidateMoves.count];
}