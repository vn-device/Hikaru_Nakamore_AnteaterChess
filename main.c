/******************************************************************************
 * File: main.c
 * Author: Vinh Nguyen
 * Date: April 03, 2026
 * 
 * * Description:
 * Main entry point and game loop state management for the interactive
 * Anteater Chess program. Orchestrates initialization, rendering, and 
 * the main execution loop for the game engine. Implements persistent I/O
 * game logging with asynchronous POSIX signal handling for safe termination,
 * and outputs terminal notifications for captures and special moves.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "GameData.h"
#include "MoveValidation.h"
#include "ChessAI.h"
#include "GUI.h"

//=============================================================================

#define SZCODEVERSION "2.0.0"
#define MAX_INPUT_LENGTH 32

/* Global file pointer required for asynchronous access within the signal handler */
static FILE* g_pLogFile = NULL;


//=============================================================================

/*
 * POSIX Signal Handler for SIGINT (Ctrl+C).
 * Intercepts the terminal interrupt, flushes the final game state to the log, 
 * closes the file descriptor to prevent corruption, and exits the process safely.
 */
void HandleSigInt(int sig)
{
    printf("\nKeyboard interruption received. Exiting safely...\n");
    if (g_pLogFile) {
        fprintf(g_pLogFile, "{Game terminated via keyboard interruption (SIGINT)}\n");
        fclose(g_pLogFile);
    }
    exit(0);
}

/*
 * Static helper to handle file I/O for game logging and terminal UX events. 
 * Must be invoked PRIOR to ApplyMove() to accurately deduce capture states.
 */
static const char* LogMove(FILE* logFile, Board* pBoard, int turnCount, char color, int fRow, int fCol, int tRow, int tCol)
{
    Piece mover  = pBoard->grid[fRow][fCol];
    Piece target = pBoard->grid[tRow][tCol];

    const char* colorStr = (color == 'w') ? "White" : "Black";
    const char* enemyStr = (color == 'w') ? "Black" : "White";

    char san[16] = "";
    int  pos = 0;

    /* castling */
    if (mover.type == 'K' && abs(tCol - fCol) == 2) {
        strcpy(san, (tCol > fCol) ? "O-O" : "O-O-O");
    }
    /* all other moves */
    else {
        /* piece letter (omitted for pawns) */
        if (mover.type != 'P') {
            san[pos++] = mover.type; // 'K','Q','R','B','N','A' etc.
        }

        /* Disambiguation for pawns on capture (FIDE rules) */
        int isCapture = (target.type != ' ' && target.color != color)
                     || (mover.type == 'P' && IsEnPassant(pBoard, fRow, fCol, tRow, tCol, color))
                     || (mover.type == 'A' && target.type == 'P' && target.color != color);

        if (mover.type == 'P' && isCapture) {
            /* pawns always show source file when capturing */
            san[pos++] = (char)('a' + fCol);
        }

        /* capture marker */
        if (isCapture) {
            san[pos++] = 'x';
        }

        /* destination line */
        san[pos++] = (char)('a' + tCol);
        san[pos++] = (char)('1' + tRow);

        /* promotion suffix */
        if (mover.type == 'P' && ((color == 'w' && tRow == ROWS - 1) || (color == 'b' && tRow == 0))) {
            san[pos++] = '=';
            san[pos++] = 'Q'; // assumes auto-queen for now
        }

        san[pos] = '\0';
        
        /* unchanged code from prev branch */
        if (mover.type == 'A' && target.type == 'P' && target.color != color) {
            printf("\n>>> %s Anteater devoured enemy ants! <<<\n", colorStr);
        }
        else if (mover.type == 'P' && IsEnPassant(pBoard, fRow, fCol, tRow, tCol, color)) {
            printf("\n>>> %s Pawn captured via En Passant! <<<\n", colorStr);
        }
        else {
            if (target.type != ' ' && target.color != color) {
                printf("\n>>> %s captured %s's %c! <<<\n", colorStr, enemyStr, target.type);
            }
            if (mover.type == 'P' && ((color == 'w' && tRow == ROWS - 1) || (color == 'b' && tRow == 0))) {
                printf("\n>>> %s Pawn was promoted to a Queen! <<<\n", colorStr);
            }
        }
    }

    if (logFile) {
        /* white move opens the pair, black will close it */
        if (color == 'w') {
            fprintf(logFile, "%d. %s ", turnCount, san);
        }
        else {
            fprintf(logFile, "%s\n", san);
        }
        fflush(logFile);
    }
    static char returnedSAN[16];
    strncpy(returnedSAN, san, sizeof(returnedSAN) - 1);
    returnedSAN[sizeof(returnedSAN) - 1] = '\0';
    return returnedSAN;
}

/*
 * Public function for GUI to process and log a move.
 * Must be called PRIOR to ApplyMove() to accurately log special moves.
 * Returns 1 if successful, 0 if move is invalid.
 */
int GUI_ProcessMove(Board* pBoard, FILE* logFile, int turnCount, char color, int fRow, int fCol, int tRow, int tCol, char* outSAN, size_t sanBufSize)
{
    /* Validate move before logging */
    if (!IsValidMove(pBoard, fRow, fCol, tRow, tCol, color)) {
        return 0;
    }
    

    /* Log the move with all special move detection */
    const char* san = LogMove(logFile, pBoard, turnCount, color, fRow, fCol, tRow, tCol);
    if (outSAN && sanBufSize > 0) {
        strncpy(outSAN, san, sanBufSize - 1);
        outSAN[sanBufSize - 1] = '\0';
    }
    
    /* Apply the move */
    ApplyMove(pBoard, fRow, fCol, tRow, tCol);
    
    return 1;
}

/*
 * Encapsulated procedural loop handling terminal I/O.
 * Isolated to prevent blocking GTK's asynchronous event handling.
 */
void RunTerminalGame(Board* pBoard, char gameMode, char aiDifficulty, char playerColor)
{
    char moveInput[MAX_INPUT_LENGTH];
    char currentTurn = 'w';
    char fromCol, toCol;
    int fromRow, toRow, gameOver = 0;
    int fullTurnCount = 1;

    while (!gameOver) {
        PrintBoard(pBoard);

        if (IsInCheck(pBoard, currentTurn)) {
            if (IsCheckmate(pBoard, currentTurn)) {
                printf("\nCHECKMATE! %s wins the game.\n", (currentTurn == 'w') ? "Black" : "White");
                if (g_pLogFile) {
                    fprintf(g_pLogFile, "{Game Over: %s wins by Checkmate}\n", (currentTurn == 'w') ? "Black" : "White");
                    fflush(g_pLogFile);
                }
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
                    int fCol = fromCol - (fromCol >= 'a' ? 'a' : 'A');
                    int fRow = fromRow - 1;
                    int tCol = toCol - (toCol >= 'a' ? 'a' : 'A');
                    int tRow = toRow - 1;

                    if ((fRow >= 0 && fRow < ROWS) && (tRow >= 0 && tRow < ROWS) && 
                        (fCol >= 0 && fCol < COLS) && (tCol >= 0 && tCol < COLS)) {
                            
                        if (IsValidMove(pBoard, fRow, fCol, tRow, tCol, currentTurn)) {
                            LogMove(g_pLogFile, pBoard, fullTurnCount, currentTurn, fRow, fCol, tRow, tCol);
                            ApplyMove(pBoard, fRow, fCol, tRow, tCol);
                            
                            if (currentTurn == 'b') fullTurnCount++;
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
                if (g_pLogFile) {
                    fprintf(g_pLogFile, "{Game terminated via keyboard interruption (EOF)}\n");
                    fflush(g_pLogFile);
                }
                gameOver = 1;
            }
        }
        else {
            Move aiMove = DetermineAIMove(pBoard, currentTurn, aiDifficulty);
            
            if (aiMove.fRow != 0 || aiMove.fCol != 0 || aiMove.tRow != 0 || aiMove.tCol != 0 || pBoard->grid[0][0].type != ' ') {
                printf("\nBot played: %c%d -> %c%d\n", 
                       (char)(aiMove.fCol + 'A'), aiMove.fRow + 1, 
                       (char)(aiMove.tCol + 'A'), aiMove.tRow + 1);

                LogMove(g_pLogFile, pBoard, fullTurnCount, currentTurn, aiMove.fRow, aiMove.fCol, aiMove.tRow, aiMove.tCol);
                ApplyMove(pBoard, aiMove.fRow, aiMove.fCol, aiMove.tRow, aiMove.tCol);
                
                if (currentTurn == 'b') fullTurnCount++;
                currentTurn = (currentTurn == 'w') ? 'b' : 'w';
            }
            else {
                gameOver = 1;
            }
        }
    }
}



int main(int argc, char *argv[])
{
    /* Register the signal handler for terminal interruptions */
    signal(SIGINT, HandleSigInt);

    /* Initializing the game board */
    Board gameBoard;
    InitializeBoard(&gameBoard);

    char uiMode = ' ', gameMode = ' ', aiDifficulty = ' ', playerColor = ' ';
    printf("Welcome to Anteater Chess by Team Hikaru Naka-more!\n");

    /* Prompt user for game display mode */  
    while (uiMode != '1' && uiMode != '2') {
        printf("\nPlease Select Your Preferred Game Display\n1. Terminal\n2. GUI\n");
        if (scanf(" %c", &uiMode) != 1) while (getchar() != '\n');
    }

    /* Prompt user for game mode (PvP / PvE) */  
    while (gameMode != '1' && gameMode != '2') {
        printf("\nPlease Select Mode\n1. Play a Friend\n2. Play Bots\n");
        if (scanf(" %c", &gameMode) != 1) while (getchar() != '\n');
    }

    if (gameMode == '1') {
        playerColor = 'w';
    }
    else if (gameMode == '2') {
        while (aiDifficulty != '1' && aiDifficulty != '2' && aiDifficulty != '3') {
            printf("\nPlease Select Difficulty\n1. Easy\n2. Medium\n3. Hard\n");
            if (scanf(" %c", &aiDifficulty) != 1) while (getchar() != '\n');
        }
    }

    /* Prompt user for piece color (PvE ONLY) */  
    while (playerColor != 'w' && playerColor != 'b') {    
        printf("\nPlease choose your side ('w' for white / 'b' for black): ");
        if (scanf(" %c", &playerColor) != 1) while (getchar() != '\n');
    }

    while (getchar() != '\n');

    g_pLogFile = fopen("game_log.txt", "w");
    if (!g_pLogFile) {
        printf("CRITICAL WARNING: Unable to open game_log.txt for writing.\n");
    }

    if (uiMode == '1') {
        RunTerminalGame(&gameBoard, gameMode, aiDifficulty, playerColor);
    }
    else {
        SetGUIGameContext(g_pLogFile, 'w', gameMode, aiDifficulty, playerColor);
        StartGUI(argc, argv, &gameBoard);
    }

    if (g_pLogFile) fclose(g_pLogFile);

    return 0;
}
