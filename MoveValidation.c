/******************************************************************************
 * File: MoveValidation.c
 * Author: Hamza Faisal
 * Date: April 09, 2026
 *
 * * Description:
 * Implements the core logic for movement validation and game-state analysis.
 * This module provides the piece-specific rule enforcement (including the 
 * unique Anteater capture mechanics and standard Chess special moves like 
 * En Passant). It also manages state detection for Check and Checkmate scenarios.
 *****************************************************************************/

#include <stdlib.h>
#include "MoveValidation.h" /* finish later */
#include "GameData.h"

//=============================================================================

/* come back to this: en passant state - -1 means not available */
int enPassantCol = -1;
int enPassantRow = -1;

void SetEnPassant(int row, int col)
{
    enPassantRow = row;
    enPassantCol = col;
}

void ClearEnPassant(void)
{
    enPassantRow = -1;
    enPassantCol = -1;
}

//=============================================================================

/* IsPathClear
 * checks that no piece sits between (fRow,fCol) and (tRow,tCol).
 * works for straight lines (rook) and diagonals (bishop/queen).*/
int IsPathClear(Board* pBoard, int fRow, int fCol, int tRow, int tCol)
{
    int dirRow = (tRow > fRow) ? 1 : (tRow < fRow ? -1 : 0);
    int dirCol = (tCol > fCol) ? 1 : (tCol < fCol ? -1 : 0);

    int r = fRow + dirRow;
    int c = fCol + dirCol;

    while (r != tRow || c != tCol) {
        if (pBoard->grid[r][c].type != ' ') {
            /* something is in the way */
            return 0;
        }
        
        r += dirRow;
        c += dirCol;
    }

    return 1;
}

/* IsValidPawn
 * white pawns move from row 1 upward (increasing row index).
 * black pawns move from row 6 downward (decreasing row index).*/
int IsValidPawn(Board* pBoard, int fRow, int fCol,
                int tRow, int tCol, char color)
{
    int dir        = (color == 'w') ? 1 : -1;
    int startRow   = (color == 'w') ? 1 : 6;

    /* one square forward into empty square */
    if (tCol == fCol && tRow == fRow + dir &&
        pBoard->grid[tRow][tCol].type == ' ')
        return 1;

    /* two squares forward from starting row into empty squares */
    if (tCol == fCol && fRow == startRow && tRow == fRow + 2 * dir
        && pBoard->grid[fRow + dir][fCol].type == ' '
        && pBoard->grid[tRow][tCol].type == ' ')
        return 1;

    /* diagonal capture of an enemy piece */
    if (tRow == fRow + dir && abs(tCol - fCol) == 1
        && pBoard->grid[tRow][tCol].type != ' '
        && pBoard->grid[tRow][tCol].color != color)
        return 1;

    /* en passant */
    if (IsEnPassant(pBoard, fRow, fCol, tRow, tCol, color))
        return 1;

    return 0;
}

/* IsValidRook
 * moves any number of squares horizontally or vertically.*/
int IsValidRook(Board* pBoard, int fRow, int fCol, int tRow, int tCol)
{
    /* must move in a straight line (not diagonal) */
    if (fRow != tRow && fCol != tCol)
        return 0;

    return IsPathClear(pBoard, fRow, fCol, tRow, tCol);
}

/* IsValidKnight
 * jumps in an L-shape: 2 squares one direction, 1 square perpendicular.
 * knights ignore pieces in the way - no path check needed.*/
int IsValidKnight(int fRow, int fCol, int tRow, int tCol)
{
    int dr = abs(tRow - fRow);
    int dc = abs(tCol - fCol);

    return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
}

/*IsValidBishop
 * moves any number of squares diagonally.*/
int IsValidBishop(Board* pBoard, int fRow, int fCol, int tRow, int tCol)
{
    /* Must move diagonally - row and column deltas must be equal */
    if (abs(tRow - fRow) != abs(tCol - fCol))
        return 0;

    return IsPathClear(pBoard, fRow, fCol, tRow, tCol);
}

/* IsValidQueen
 * combines rook and bishop movement.*/
int IsValidQueen(Board* pBoard, int fRow, int fCol, int tRow, int tCol)
{
    return IsValidRook  (pBoard, fRow, fCol, tRow, tCol)
        || IsValidBishop(pBoard, fRow, fCol, tRow, tCol);
}

/* IsValidKing
 * moves exactly one square in any direction.
 * TODO: Add castling support here (check hasMoved flags).*/
int IsValidKing(int fRow, int fCol, int tRow, int tCol)
{
    int dr = abs(tRow - fRow);
    int dc = abs(tCol - fCol);

    return (dr <= 1 && dc <= 1 && (dr + dc) > 0);
}

/* fix later: IsValidAnteater
 * normal move: one step in any direction (like a king) to an empty square.
 * capture move: eats a straight line of adjacent pawns (ants).
 *   - the first target must be a pawn adjacent to the anteater.
 *   - the anteater then eats along the same rank or file until the chosen endpoint, which must also be a pawn.
 *   - the endpoint is where the anteater lands. */
int IsValidAnteater(Board* pBoard, int fRow, int fCol,
                    int tRow, int tCol, char color)
{
    int dr = abs(tRow - fRow);
    int dc = abs(tCol - fCol);
    char enemy = (color == 'w') ? 'b' : 'w';

    /* normal king-like move to an empty square */
    if (dr <= 1 && dc <= 1 && (dr + dc) > 0
        && pBoard->grid[tRow][tCol].type == ' ')
        return 1;

    /* ant-eating capture:
     * must be a straight line (same rank or same file).
     * every square from the first adjacent square up to and including
     * the destination must contain an enemy pawn. */
    if (fRow != tRow && fCol != tCol)
        return 0; /* Diagonal eating not allowed */

    int dirRow = (tRow > fRow) ? 1 : (tRow < fRow ? -1 : 0);
    int dirCol = (tCol > fCol) ? 1 : (tCol < fCol ? -1 : 0);

    /* the first square eaten must be adjacent to the anteater */
    int firstR = fRow + dirRow;
    int firstC = fCol + dirCol;

    if (pBoard->grid[firstR][firstC].color != enemy
        || pBoard->grid[firstR][firstC].type  != 'P')
        return 0;

    /* walk the line - every square up to destination must be an enemy pawn */
    int r = firstR;
    int c = firstC;
    while (r != tRow || c != tCol) {
        if (pBoard->grid[r][c].color != enemy
            || pBoard->grid[r][c].type  != 'P')
            return 0;
        
        r += dirRow;
        c += dirCol;
    }

    /* destination must also be an enemy pawn */
    if (pBoard->grid[tRow][tCol].color != enemy
        || pBoard->grid[tRow][tCol].type  != 'P')
        return 0;

    return 1;
}

/* IsEnPassant*/
int IsEnPassant(Board* pBoard, int fRow, int fCol,
                int tRow, int tCol, char color)
{
    if (enPassantCol == -1)
        return 0;

    int dir = (color == 'w') ? 1 : -1;

    /* moving diagonally forward into the en passant square */
    if (tRow == fRow + dir && abs(tCol - fCol) == 1
        && tCol == enPassantCol && fRow == enPassantRow
        && pBoard->grid[fRow][enPassantCol].color != color
        && pBoard->grid[fRow][enPassantCol].type  == 'P')
        return 1;

    return 0;
}

/*IsInCheck
 * returns 1 if the king of 'color' is under attack by any enemy piece.*/
int IsInCheck(Board* pBoard, char color)
{
    /* find the king */
    int kingRow = -1, kingCol = -1;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (pBoard->grid[r][c].color == color
                && pBoard->grid[r][c].type  == 'K') {
                kingRow = r;
                kingCol = c;
            }
        }
    }

    if (kingRow == -1) {
        /* king not found: shouldn't happen */
        return 0;
    }

    char enemy = (color == 'w') ? 'b' : 'w';

    /* check if any enemy piece can reach the king's square */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (pBoard->grid[r][c].color != enemy) continue;

            char type = pBoard->grid[r][c].type;
            int attacks = 0;

            switch (type) {
                case 'P': attacks = IsValidPawn  (pBoard, r, c, kingRow, kingCol, enemy); break;
                case 'R': attacks = IsValidRook  (pBoard, r, c, kingRow, kingCol);        break;
                case 'N': attacks = IsValidKnight(r, c, kingRow, kingCol);                break;
                case 'B': attacks = IsValidBishop(pBoard, r, c, kingRow, kingCol);        break;
                case 'Q': attacks = IsValidQueen (pBoard, r, c, kingRow, kingCol);        break;
                case 'K': attacks = IsValidKing  (r, c, kingRow, kingCol);                break;
                /* anteater cannot threaten the king */
                default: break;
            }

            if (attacks) return 1;
        }
    }

    return 0;
}

/* IsValidMove (top-level dispatcher)
 * recheck logic: call this from main.c before every MovePiece().*/
int IsValidMove(Board* pBoard, int fRow, int fCol,
                int tRow, int tCol, char color)
{
    Piece mover = pBoard->grid[fRow][fCol];
    Piece target = pBoard->grid[tRow][tCol];

    /* must be moving your own piece */
    if (mover.color != color || mover.type == ' ')
        return 0;

    /* cannot capture your own piece */
    if (target.color == color)
        return 0;

    /* check piece-specific rules */
    int legal = 0;
    switch (mover.type) {
        case 'P': legal = IsValidPawn    (pBoard, fRow, fCol, tRow, tCol, color); break;
        case 'R': legal = IsValidRook    (pBoard, fRow, fCol, tRow, tCol);        break;
        case 'N': legal = IsValidKnight  (fRow, fCol, tRow, tCol);                break;
        case 'B': legal = IsValidBishop  (pBoard, fRow, fCol, tRow, tCol);        break;
        case 'Q': legal = IsValidQueen   (pBoard, fRow, fCol, tRow, tCol);        break;
        case 'K': legal = IsValidKing    (fRow, fCol, tRow, tCol);                break;
        case 'A': legal = IsValidAnteater(pBoard, fRow, fCol, tRow, tCol, color); break;
        default:  break;
    }

    if (!legal) return 0;

    /* simulate the move and make sure it doesn't leave our king in check */
    Board temp = *pBoard;
    temp.grid[tRow][tCol] = temp.grid[fRow][fCol];
    temp.grid[fRow][fCol].color = ' ';
    temp.grid[fRow][fCol].type  = ' ';

    if (IsInCheck(&temp, color))
        return 0;

    return 1;
}

/* IsCheckmate
 * returns 1 if 'color' has no legal move available.*/
int IsCheckmate(Board* pBoard, char color)
{
    for (int fRow = 0; fRow < ROWS; fRow++) {
        for (int fCol = 0; fCol < COLS; fCol++) {
            if (pBoard->grid[fRow][fCol].color != color) continue;

            for (int tRow = 0; tRow < ROWS; tRow++) {
                for (int tCol = 0; tCol < COLS; tCol++) {
                    if (IsValidMove(pBoard, fRow, fCol, tRow, tCol, color))
                        return 0; /* At least one legal move exists */
                }
            }
        }
    }
    
    return 1; /* no legal moves: checkmate (or stalemate) - figure out stalemate */
}

void ApplyMove(Board* pBoard, int fRow, int fCol, int tRow, int tCol)
{
    Piece mover = pBoard->grid[fRow][fCol];

    /* en passant capture */
    if (mover.type == 'P' && IsEnPassant(pBoard, fRow, fCol, tRow, tCol, mover.color)) {
        MovePiece(pBoard, fRow, fCol, tRow, tCol);

        if (mover.color == 'w') {
            pBoard->grid[tRow - 1][tCol].color = ' ';
            pBoard->grid[tRow - 1][tCol].type = ' ';
        }
        else {
            pBoard->grid[tRow + 1][tCol].color = ' ';
            pBoard->grid[tRow + 1][tCol].type = ' ';
        }

        ClearEnPassant();
        return;
    }

    /* mark en passant chance after a 2-square pawn move */
    if (mover.type == 'P' && abs(tRow - fRow) == 2) {
        SetEnPassant(tRow, tCol);
    }
    else {
        ClearEnPassant();
    }

    /* anteater special capture */
    if (mover.type == 'A' && pBoard->grid[tRow][tCol].type == 'P' &&
        pBoard->grid[tRow][tCol].color != mover.color) {
        AnteaterCapture(pBoard, fRow, fCol, tRow, tCol, mover.color);
        return;
    }

    /* normal move */
    MovePiece(pBoard, fRow, fCol, tRow, tCol);

    /* simple pawn promotion */
    if (mover.type == 'P') {
        if ((mover.color == 'w' && tRow == ROWS - 1) ||
            (mover.color == 'b' && tRow == 0)) {
            pBoard->grid[tRow][tCol].type = 'Q';
        }
    }
}