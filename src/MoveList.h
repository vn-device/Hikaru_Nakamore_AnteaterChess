#ifndef MOVELIST_H
#define MOVELIST_H

#include "GameData.h"

#define MAX_MOVES 1024

typedef struct {
    int fRow;
    int fCol;
    int tRow;
    int tCol;
} Move;

typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

void InitMoveList(MoveList* pList);
int AddMove(MoveList* pList, int fRow, int fCol, int tRow, int tCol);
void GenerateLegalMoves(Board* pBoard, char color, MoveList* pList);

#endif