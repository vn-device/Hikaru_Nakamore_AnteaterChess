#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include <stdio.h>
#include "GameData.h"

/* Initializes the GTK environment, constructs the main window using GtkGrid, 
 * and enters the event-driven GTK main loop. 
 */
void StartGUI(int argc, char *argv[], Board* pBoard);

/*
 * Sets up game context for the GUI (log file and initial player)
 * Should be called before or after StartGUI to sync game state
 */
void SetGUIGameContext(FILE* logFile, char startingPlayer, char gameMode, char aiDifficulty, char playerColor);
/*
 * Public function for GUI to process and log a move.
 * Validates move, logs it with special move detection, and applies it.
 * Returns 1 if successful, 0 if move is invalid.
 */
int GUI_ProcessMove(Board* pBoard, FILE* logFile, int turnCount, char color, int fRow, int fCol, int tRow, int tCol, char* outSAN, size_t sanBufSize);

#endif // GUI_H
