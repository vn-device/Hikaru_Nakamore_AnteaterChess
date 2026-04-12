/******************************************************************************
 * File: GameData.c
 * Author: Vinh Nguyen
 * Date: April 03, 2026
 * 
 * * Description:
 * Implements the core board manipulation and rendering functions. 
 * Handles the memory initialization of the 8x10 board matrix to the 
 * Anteater Chess starting state and handles terminal-based ASCII 
 * grid output.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GameData.h"

//=============================================================================

void InitializeBoard(Board* pBoard)
{
    /* Initialize the board with no pieces w/ no color */
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            pBoard->grid[row][col].color = ' ';
            pBoard->grid[row][col].type = ' ';
        }
    }

    /* Set up pawns for both colors */
    for (int col = 0; col < COLS; col++) {
        pBoard->grid[1][col].color = 'w';
        pBoard->grid[1][col].type = 'P';

        pBoard->grid[6][col].color = 'b';
        pBoard->grid[6][col].type = 'P';
    }

    /* Set up higher level pieces for both colors */
    char pieceOrder[10] = {'R', 'N', 'B', 'A', 'Q', 'K', 'A', 'B', 'N', 'R'};

    for (int col = 0; col < COLS; col++) {
        pBoard->grid[0][col].color = 'w';
        pBoard->grid[0][col].type = pieceOrder[col];

        pBoard->grid[7][col].color = 'b';
        pBoard->grid[7][col].type = pieceOrder[col];
    }
}

//=============================================================================

void PrintBoard(Board* pBoard)
{
    printf("\n");
    for (int row = ROWS - 1; row >= 0; row--) {
        /* Print top border of the row */
        printf("  +----+----+----+----+----+----+----+----+----+----+\n");
        
        /* Print row number (1-indexed for display) */
        printf("%d |", row + 1);
        
        /* Print pieces */
        for (int col = 0; col < COLS; col++) {
            Piece p = pBoard->grid[row][col];
            if (p.type == ' ') {
                printf("    |");
            }
            else {
                printf(" %c%c |", p.color, p.type);
            }
        }
        printf("\n");
    }

    /* Print bottom border and column letters */
    printf("  +----+----+----+----+----+----+----+----+----+----+\n");
    printf("    A    B    C    D    E    F    G    H    I    J\n\n");
}

//=============================================================================

void MovePiece(Board* pBoard, int oRank, int oFile, int nRank, int nFile)
{
    pBoard->grid[nRank][nFile] = pBoard->grid[oRank][oFile];
    pBoard->grid[oRank][oFile].color = ' ';
    pBoard->grid[oRank][oFile].type = ' ';
}

//=============================================================================

void AnteaterCapture(Board* pBoard, int oRank, int oFile, int nRank, int nFile, char color)
{
    int r = oRank;
    int f = oFile;

    /* Determine direction of the 'hunger' (Horizontal vs Vertical) */
    int dirR = (nRank > oRank) ? 1 : (nRank < oRank ? -1 : 0);
    int dirF = (nFile > oFile) ? 1 : (nFile < oFile ? -1 : 0);

    /* The anteater itself starts at oRank/oFile. 
       We clear every ant from the start up to the chosen end position. */
    while (1) {
        /* Clear the square (Eating the ant) */
        pBoard->grid[r][f].color = ' ';
        pBoard->grid[r][f].type = ' ';

        if (r == nRank && f == nFile) break;

        r += dirR;
        f += dirF;
    }

    /* Place the anteater at the final position eaten */
    pBoard->grid[nRank][nFile].color = color;
    pBoard->grid[nRank][nFile].type = 'A';
}