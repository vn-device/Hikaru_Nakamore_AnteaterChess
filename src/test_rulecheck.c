/******************************************************************************
 * File: test_rulecheck.c
 * Author: Team Hikaru Naka-more
 * Date: April 19, 2026
 * 
 * * Description:
 * Unit test driver to validate the rules engine implementation.
 * Verifies that the IsValidMove function correctly permits legal moves 
 * and rejects illegal paths, blocked paths, or bound violations.
 *****************************************************************************/

#include <stdio.h>
#include "GameData.h"
#include "MoveValidation.h"

int main()
{
    Board testBoard;
    InitializeBoard(&testBoard);
    
    printf("--- Executing Rule Validation Tests ---\n");

    /* Test 1: Legal White Pawn Move (Double Step Forward) 
     * From E2 (Row 1, Col 4) to E4 (Row 3, Col 4) */
    if (IsValidMove(&testBoard, 1, 4, 3, 4, 'w')) {
        printf("[TEST 1] Attempt White Pawn Double Step (E2 -> E4) - Legal Move\n");
    }
    else {
        printf("[TEST 1] Attempt White Pawn Double Step (E2 -> E4) - Illegal Move\n");
    }
    
    /* Apply the move to set up subsequent board state tests */
    ApplyMove(&testBoard, 1, 4, 3, 4);

    /* Test 2: Illegal White Pawn Move (Moving backward) 
     * From E4 (Row 3, Col 4) to E3 (Row 2, Col 4) */
    if (IsValidMove(&testBoard, 3, 4, 2, 4, 'w')) {
        printf("[TEST 2] Attempt White Pawn moving backward (E4 -> E3) - Legal Move\n");
    }
    else {
        printf("[TEST 2] Attempt White Pawn moving backward (E4 -> E3) - Illegal Move\n");
    }

    /* Test 3: Illegal White Rook Move (Blocked by Pawn) 
     * From A1 (Row 0, Col 0) to A3 (Row 2, Col 0) */
    if (IsValidMove(&testBoard, 0, 0, 2, 0, 'w')) {
        printf("[TEST 3] Attempt White Rook path blocked by own Pawn (A1 -> A3) - Legal Move\n");
    }
    else {
        printf("[TEST 3] Attempt White Rook path blocked by own Pawn (A1 -> A3) - Illegal Move\n");
    }

    /* Test 4: Legal White Knight Move (Jumps over Pawn array)
     * From B1 (Row 0, Col 1) to C3 (Row 2, Col 2) */
    if (IsValidMove(&testBoard, 0, 1, 2, 2, 'w')) {
        printf("[TEST 4] Attempt White Knight jump over pawns (B1 -> C3) - Legal Move\n");
    }
    else {
        printf("[TEST 4] Attempt White Knight jump over pawns (B1 -> C3) - Illegal Move\n");
    }

    /* Test 5: Rule Violation (Wrong Color)
     * White attempting to manipulate Black's Pawn at A7 (Row 6, Col 0) */
    if (IsValidMove(&testBoard, 6, 0, 5, 0, 'w')) {
        printf("[TEST 5] Attempt White attempting to move a Black piece (A7) - Legal Move\n");
    }
    else {
        printf("[TEST 5] Attempt White attempting to move a Black piece (A7) - Illegal Move\n");
    }

    /* Test 6: Illegal Diagonal Move into Empty Space
     * White Pawn at A2 (Row 1, Col 0) attempting to move to B3 (Row 2, Col 1) */
    if (IsValidMove(&testBoard, 1, 0, 2, 1, 'w')) {
        printf("[TEST 6] Attempt White Pawn diagonal into empty space (A2 -> B3) - Legal Move\n");
    }
    else {
        printf("[TEST 6] Attempt White Pawn diagonal into empty space (A2 -> B3) - Illegal Move\n");
    }

    printf("--- Rule Validation Tests Complete ---\n");
    return 0;
}