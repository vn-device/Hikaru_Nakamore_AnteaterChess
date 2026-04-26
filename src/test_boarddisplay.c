/******************************************************************************
 * File: test_boarddisplay.c
 * Author: Team Hikaru Naka-more
 * Date: April 19, 2026
 * 
 * * Description:
 * Unit test to verify the initialization and terminal ASCII rendering 
 * of the Anteater Chess board, including state modifications.
 *****************************************************************************/

#include <stdio.h>
#include "GameData.h"
#include "MoveValidation.h"

int main()
{
    Board testBoard;
    
    printf("--- Executing Board Display Test ---\n");
    
    /* 1. Test Initial Board State */
    printf("[TEST 1] Initializing Standard Board State:\n");
    InitializeBoard(&testBoard);
    PrintBoard(&testBoard);
    
    /* 2. Test Modified Board State */
    printf("[TEST 2] Applying sample moves and re-rendering:\n");
    
    /* Move White Pawn from E2 (Row 1, Col 4) to E4 (Row 3, Col 4) */
    ApplyMove(&testBoard, 1, 4, 3, 4);
    
    /* Move Black Pawn from E7 (Row 6, Col 4) to E5 (Row 4, Col 4) */
    ApplyMove(&testBoard, 6, 4, 4, 4);
    
    /* Move White Knight from B1 (Row 0, Col 1) to C3 (Row 2, Col 2) */
    ApplyMove(&testBoard, 0, 1, 2, 2);
    
    PrintBoard(&testBoard);
    
    printf("--- Board Display Test Complete ---\n");
    return 0;
}