#ifndef MOVEVALIDATION_H
#define MOVEVALIDATION_H

#include "GameData.h"

void SetEnPassant(int row, int col);
void ClearEnPassant(void);

int IsPathClear(Board* pBoard, int fRow, int fCol, int tRow, int tCol);
int IsValidPawn(Board* pBoard, int fRow, int fCol, int tRow, int tCol, char color);
int IsValidRook(Board* pBoard, int fRow, int fCol, int tRow, int tCol);
int IsValidKnight(int fRow, int fCol, int tRow, int tCol);
int IsValidBishop(Board* pBoard, int fRow, int fCol, int tRow, int tCol);
int IsValidQueen(Board* pBoard, int fRow, int fCol, int tRow, int tCol);
int IsValidKing(int fRow, int fCol, int tRow, int tCol);
int IsValidAnteater(Board* pBoard, int fRow, int fCol, int tRow, int tCol, char color);
int IsEnPassant(Board* pBoard, int fRow, int fCol, int tRow, int tCol, char color);
int IsInCheck(Board* pBoard, char color);
int IsValidMove(Board* pBoard, int fRow, int fCol, int tRow, int tCol, char color);
int IsCheckmate(Board* pBoard, char color);

/* add-on function so main.c does not call MovePiece directly */
void ApplyMove(Board* pBoard, int fRow, int fCol, int tRow, int tCol);

#endif