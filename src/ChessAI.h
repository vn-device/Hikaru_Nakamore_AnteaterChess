/******************************************************************************
 * File: ChessAI.h
 * Author: Jonghyun Choi, Vinh Nguyen
 * Date: April 14, 2026
 *
 * Description:
 * Declares the public API for the computer player (Chess Bot) strategy module.
 * Exposes functions for static board evaluation and determining the optimal 
 * move using a unified Minimax algorithm with Alpha-Beta pruning.
 *****************************************************************************/

#ifndef CHESSAI_H
#define CHESSAI_H

#include "GameData.h"
#include "MoveList.h"

//=============================================================================

enum VAL_PIECE {
    VAL_PAWN     = 100,
    VAL_KNIGHT   = 300,
    VAL_BISHOP   = 300,
    VAL_ANTEATER = 400,
    VAL_ROOK     = 500,
    VAL_QUEEN    = 900,
    VAL_KING     = 10000
};

/* * Primary entry point for the computer player.
 * Evaluates the board state and returns the optimal Move struct based 
 * on the specified difficulty level ('1'=Easy, '2'=Medium, '3'=Hard).
 */
Move DetermineAIMove(Board* pBoard, char aiColor, char difficulty);

/* Static evaluation function: Scores the board based on material advantage */
int EvaluateBoard(Board* pBoard, char aiColor);

#endif // CHESSAI_H