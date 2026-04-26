#include "MoveList.h"
#include "MoveValidation.h"

void InitMoveList(MoveList* pList)
{
    pList->count = 0;
}

int AddMove(MoveList* pList, int fRow, int fCol, int tRow, int tCol)
{
    if (pList->count >= MAX_MOVES) {
        return 0;
    }

    pList->moves[pList->count].fRow = fRow;
    pList->moves[pList->count].fCol = fCol;
    pList->moves[pList->count].tRow = tRow;
    pList->moves[pList->count].tCol = tCol;
    pList->count++;
    return 1;
}

void GenerateLegalMoves(Board* pBoard, char color, MoveList* pList)
{
    int fRow;
    int fCol;
    int tRow;
    int tCol;

    InitMoveList(pList);

    for (fRow = 0; fRow < ROWS; fRow++) {
        for (fCol = 0; fCol < COLS; fCol++) {
            if (pBoard->grid[fRow][fCol].color != color) {
                continue;
            }

            for (tRow = 0; tRow < ROWS; tRow++) {
                for (tCol = 0; tCol < COLS; tCol++) {
                    if (IsValidMove(pBoard, fRow, fCol, tRow, tCol, color)) {
                        if (!AddMove(pList, fRow, fCol, tRow, tCol)) {
                            return;
                        }
                    }
                }
            }
        }
    }
}