/******************************************************************************
 * File: main.c
 * Author: Vinh Nguyen
 * Date: April 03, 2026
 * 
 * * Description:
 * Main entry point and game loop state management for the interactive
 * Anteater Chess program. Orchestrates initialization, rendering, and 
 * the main execution loop for the game engine.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GameData.h"
#include "MoveValidation.h"

//=============================================================================

#define SZCODEVERSION "1.0.0"
#define MAX_INPUT_LENGTH 32

//=============================================================================

int main()
{
    /* Initializing the game board */
    Board gameBoard;
    InitializeBoard(&gameBoard);

    /* Prompts user for mode selection */
    char playerColor = ' ', gameMode = ' ', aiDifficulty = ' ';
    printf("Welcome to Anteater Chess by Team Hikaru Naka-more!\n");

    while (gameMode != '1' && gameMode != '2') {
        printf("\nPlease Select Mode\n1. Play a Friend\n2. Play Bots\n");

        /* Handle invalid input and clear stdin buffer */
        if (scanf(" %c", &gameMode) != 1) {
            while (getchar() != '\n');
        }
    }

    /* Skip side selection if PvP */
    if (gameMode == '1') {
        playerColor = 'w';
    }
    /* Prompts user for ai difficulty (to be used later) */
    else if (gameMode == '2') {
        while (aiDifficulty != '1' && aiDifficulty != '2' && aiDifficulty != '3') {
            printf("\nPlease Select Difficulty\n1. Easy\n2. Medium\n3. Hard\n");

            if (scanf(" %c", &aiDifficulty) != 1) {
                while (getchar() != '\n');
            }
        }
    }

    /* Prompt the user for side selection */
    while (playerColor != 'w' && playerColor != 'b') {    
        printf("\nPlease choose your side ('w' for white / 'b' for black): ");
        
        if (scanf(" %c", &playerColor) != 1) {
            while (getchar() != '\n');
        }
    }

    /* Clean up dangling \n after successful scanf */
    while (getchar() != '\n');

    /* Setup game variables */
    char moveInput[MAX_INPUT_LENGTH];
    char currentTurn = 'w';
    char fromCol, toCol;
    int fromRow, toRow, gameOver = 0;

    /* Begin game */
    while (!gameOver) {
        PrintBoard(&gameBoard);

        /* Pre-turn State Evaluation */
        if (IsInCheck(&gameBoard, currentTurn)) {
            if (IsCheckmate(&gameBoard, currentTurn)) {
                printf("\nCHECKMATE! %s wins the game.\n", (currentTurn == 'w') ? "Black" : "White");
                gameOver = 1;
                break;
            }
            printf("\nWARNING: %s is in CHECK!\n", (currentTurn == 'w') ? "White" : "Black");
        }

        int humanTurn = (gameMode == '1' || currentTurn == playerColor);
        if (humanTurn) {
            printf("\n%s's turn. Enter your move (e.g., 'E2 E4'): ",  (currentTurn == 'w') ? "White" : "Black");
            
            if (fgets(moveInput, sizeof(moveInput), stdin) != NULL) {
                if (sscanf(moveInput, " %c%d %c%d", &fromCol, &fromRow, &toCol, &toRow) == 4) {
                    /* ASCII conversion */
                    int fCol = fromCol - (fromCol >= 'a' ? 'a' : 'A');
                    int fRow = fromRow - 1;
                    int tCol = toCol - (toCol >= 'a' ? 'a' : 'A');
                    int tRow = toRow - 1;

                    /* Validate bounds before accessing board array */
                    if ((fRow >= 0 && fRow < ROWS) && (tRow >= 0 && tRow < ROWS) && 
                        (fCol >= 0 && fCol < COLS) && (tCol >= 0 && tCol < COLS)) {
                            
                        if (IsValidMove(&gameBoard, fRow, fCol, tRow, tCol, currentTurn)) {
                            ApplyMove(&gameBoard, fRow, fCol, tRow, tCol);
                            currentTurn = (currentTurn == 'w') ? 'b' : 'w';
                        }
                        else {
                            printf("Illegal move. Try again.\n");
                        }
                    }
                    else {
                        printf("Error: Coordinates out of bounds.\n");
                    }
                }
                else {
                    printf("Invalid input. Please try again.\n");
                }
            }
            else {
                printf("\nInput stream closed. Exiting game.\n");
                gameOver = 1;
            }
        }
        else {
            /* * AI Integration Point: 
             * Must populate fRow, fCol, tRow, tCol using MoveList.c logic 
             * before calling ApplyMove()
             */
            printf("Bot Thinking... \n");
            
            /* Temporary break to prevent infinite loop while AI is unwritten */
            printf("AI module not implemented. Exiting.\n");
            gameOver = 1; 
        }
    }

    return 0;
}
