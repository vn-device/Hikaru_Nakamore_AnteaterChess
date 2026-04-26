/******************************************************************************
 * File: GameData.h
 * Author: Vinh Nguyen
 * Date: April 03, 2026
 * 
 * * Description:
 * Defines the core data structures for Anteater Chess, including the 
 * 8x10 board representation and piece definitions. Declares the public 
 * API for board initialization and command-line ASCII rendering.
 *****************************************************************************/

#ifndef GAMEDATA_H
#define GAMEDATA_H

#define ROWS 8
#define COLS 10

//=============================================================================

typedef struct {
    char color;
    char type;

    /* Required for castling validation */
    unsigned char hasMoved;
} Piece;

typedef struct {
    Piece grid[ROWS][COLS];

    /* To track the row & col of En Passant vulnerable pawns */
    int epRow;
    int epCol;
} Board;

//=============================================================================

void InitializeBoard(Board* pBoard);
void PrintBoard(Board* pBoard);
void MovePiece(Board* pBoard, int oRank, int oFile, int nRank, int nFile);
void AnteaterCapture(Board* pBoard, int oRank, int oFile, int nRank, int nFile, char color);

//=============================================================================

#endif // GAMEDATA_H