#include "GUI.h"
#include <cairo.h>
#include <string.h>
#include "MoveValidation.h"
#include "ChessAI.h"

static char g_gameMode = ' ';  
static char g_aiDifficulty = ' ';
static char g_humanColor = ' ';
static int g_gameOver = 0;

static GtkWidget* g_pBoardArea = NULL;
static Board* g_pActiveBoard = NULL;

/* Game context for move logging and turn tracking */
static FILE* g_pLogFile = NULL;
static char g_currentPlayer = 'w';
static int g_fullTurnCount = 1;

/* Cached pixbufs for all pieces */
static GdkPixbuf* g_pieceBuf[2][7] = {{NULL}};  /* [color][type] */

/* Game state for tracking selected piece for GUI highlighting */
typedef struct {
    int selectedRow;
    int selectedCol;
    int validMoves[ROWS * COLS][2];  /* Array of valid move coordinates */
    int validMoveCount;
} GUIState;

static GUIState g_guiState = {-1, -1, {}, 0};

/* Reference to the move log text view */
static GtkWidget* g_pLogTextView = NULL;

/*
 * Helper function to load a piece image from the resources folder.
 * Returns an unscaled GdkPixbuf or NULL if the image cannot be loaded.
 * Images are cached and rescaled dynamically during rendering.
 */
static GdkPixbuf* LoadPieceImage(char color, char type)
{
    char filename[256];
    const char *colorName = (color == 'w') ? "white" : "black";
    const char *pieceName = NULL;
    
    /* Map piece type to piece name */
    switch (type) {
        case 'K': pieceName = "King"; break;
        case 'Q': pieceName = "Queen"; break;
        case 'R': pieceName = "Rook"; break;
        case 'B': pieceName = "Bishop"; break;
        case 'N': pieceName = "Knight"; break;
        case 'P': pieceName = "Pawn"; break;
        case 'A': pieceName = "Anteater"; break;
        default: return NULL;
    }
    
    /* Build filename: colorPiece.png (e.g., whitePawn.png, blackBishop.png) */
    snprintf(filename, sizeof(filename), "resources/%s%s.png", colorName, pieceName);
    
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    
    if (pixbuf == NULL) {
        if (error != NULL) {
            g_error_free(error);
        }
        return NULL;
    }
    
    return pixbuf;
}


/*
 * Helper function to render a single piece on the board
 */
static void RenderPiece(cairo_t *cr, Piece piece, int row, int col, double cellWidth, double cellHeight)
{
    if (piece.type == ' ') return;
    
    int colorIdx = (piece.color == 'w') ? 0 : 1;

    int typeIdx;
    switch (piece.type) {
        case 'K': typeIdx = 0; break;
        case 'Q': typeIdx = 1; break;
        case 'R': typeIdx = 2; break;
        case 'B': typeIdx = 3; break;
        case 'N': typeIdx = 4; break;
        case 'P': typeIdx = 5; break;
        case 'A': typeIdx = 6; break;
        default: return;
    }
    
    if (typeIdx < 0 || colorIdx < 0) return;
    
    /* Load and cache the piece image */
    if (g_pieceBuf[colorIdx][typeIdx] == NULL) {
        g_pieceBuf[colorIdx][typeIdx] = LoadPieceImage(piece.color, piece.type);
    }
    
    GdkPixbuf *originalPixbuf = g_pieceBuf[colorIdx][typeIdx];
    if (originalPixbuf == NULL) return;
    
    /* Calculate target size (Anteater is larger due to 512x512 source image) */
    int targetSize = (piece.type == 'A') ? 
        (int)(cellHeight * 0.9) : (int)(cellHeight * 0.8);
    
    /* Scale the pixbuf to target size */
    int originalWidth = gdk_pixbuf_get_width(originalPixbuf);
    int originalHeight = gdk_pixbuf_get_height(originalPixbuf);
    double scale = (double)targetSize / ((originalWidth > originalHeight) ? originalWidth : originalHeight);
    
    int scaledWidth = (int)(originalWidth * scale);
    int scaledHeight = (int)(originalHeight * scale);
    
    GdkPixbuf *scaledPixbuf = gdk_pixbuf_scale_simple(originalPixbuf, scaledWidth, scaledHeight, GDK_INTERP_BILINEAR);
    
    if (scaledPixbuf != NULL) {
        int renderRow = ROWS - 1 - row;
        double cellX = col * cellWidth;
        double cellY = renderRow * cellHeight;
        
        /* Center the piece in its cell */
        double x = cellX + (cellWidth - scaledWidth) / 2;
        double y = cellY + (cellHeight - scaledHeight) / 2;
        
        gdk_cairo_set_source_pixbuf(cr, scaledPixbuf, x, y);
        cairo_paint(cr);
        
        g_object_unref(scaledPixbuf);
    }
}


/*
 * Helper function to append text to the move log
 */
static void AppendToMoveLog(const char* text)
{
    if (g_pLogTextView == NULL) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_pLogTextView));
    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(buffer, &endIter);
    gtk_text_buffer_insert(buffer, &endIter, text, -1);
    
    /* Scroll to the end of the text view */
    GtkTextMark *endMark = gtk_text_buffer_get_mark(buffer, "insert");
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(g_pLogTextView), endMark);
}

/*
 * Calculates all valid moves for the selected piece
 */
static void CalculateValidMoves(void)
{
    g_guiState.validMoveCount = 0;
    
    if (g_pActiveBoard == NULL || g_guiState.selectedRow < 0) return;
    
        Piece selectedPiece = g_pActiveBoard->grid[g_guiState.selectedRow][g_guiState.selectedCol];
        if (selectedPiece.type == ' ') return;
    
    MoveList list;
    InitMoveList(&list);

    /* Generate ALL legal moves for the current player */
    GenerateLegalMoves(g_pActiveBoard, g_currentPlayer, &list);

    /* Filter moves for the selected piece only */
    for (int i = 0; i < list.count; i++) {
        if (list.moves[i].fRow == g_guiState.selectedRow &&
            list.moves[i].fCol == g_guiState.selectedCol) {

            g_guiState.validMoves[g_guiState.validMoveCount][0] = list.moves[i].tRow;
            g_guiState.validMoves[g_guiState.validMoveCount][1] = list.moves[i].tCol;
            g_guiState.validMoveCount++;
        }
    }
}

static void AdvanceTurn(void)
{
    if (g_currentPlayer == 'b') {
        g_fullTurnCount++;
    }
    g_currentPlayer = (g_currentPlayer == 'w') ? 'b' : 'w';
}

static void ShowMessage(const char* text)
{
    GtkWidget *toplevel = NULL;

    if (g_pBoardArea != NULL) {
        GtkWidget *parent = gtk_widget_get_toplevel(g_pBoardArea);
        if (GTK_IS_WINDOW(parent)) {
            toplevel = parent;
        }
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        toplevel ? GTK_WINDOW(toplevel) : NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        text
    );

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void CheckGUIEndState(void)
{
    if (g_pActiveBoard == NULL || g_gameOver) return;

    if (IsInCheck(g_pActiveBoard, g_currentPlayer)) {
        if (IsCheckmate(g_pActiveBoard, g_currentPlayer)) {
            char msg[128];
            snprintf(msg, sizeof(msg), "CHECKMATE! %s wins.",
                     (g_currentPlayer == 'w') ? "Black" : "White");

            AppendToMoveLog(msg);
            AppendToMoveLog("\n");

            if (g_pLogFile) {
                fprintf(g_pLogFile, "{Game Over: %s wins by Checkmate}\n",
                        (g_currentPlayer == 'w') ? "Black" : "White");
                fflush(g_pLogFile);
            }

            g_gameOver = 1;
            ShowMessage(msg);
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s is in CHECK!",
                     (g_currentPlayer == 'w') ? "White" : "Black");
            AppendToMoveLog(msg);
            AppendToMoveLog("\n");
        }
    }
}

static void RunAIMoveIfNeeded(void)
{
    if (g_gameOver || g_gameMode != '2' || g_currentPlayer == g_humanColor || g_pActiveBoard == NULL) {
        return;
    }

    Move aiMove = DetermineAIMove(g_pActiveBoard, g_currentPlayer, g_aiDifficulty);

    char san[16] = "";
    if (GUI_ProcessMove(g_pActiveBoard, g_pLogFile, g_fullTurnCount, g_currentPlayer,
                        aiMove.fRow, aiMove.fCol, aiMove.tRow, aiMove.tCol,
                        san, sizeof(san))) {
        char moveStr[64];
        if (g_currentPlayer == 'w') {
            snprintf(moveStr, sizeof(moveStr), "%d. %s ", g_fullTurnCount, san);
        } else {
            snprintf(moveStr, sizeof(moveStr), "%s\n", san);
        }
        AppendToMoveLog(moveStr);
        AdvanceTurn();
        CheckGUIEndState();

        if (g_pBoardArea != NULL) {
            gtk_widget_queue_draw(g_pBoardArea);
        }
    } else {
        g_gameOver = 1;
        ShowMessage("AI has no legal move.");
    }
}

/*
 * Handles board clicks to select pieces and make moves
 */
static void HandleBoardClick(int row, int col)
{
    if (g_gameOver || row < 0 || row >= ROWS || col < 0 || col >= COLS || g_pActiveBoard == NULL) return;
    
    Piece clickedPiece = g_pActiveBoard->grid[row][col];
    
    /* If a piece is already selected, try to move it */
    if (g_guiState.selectedRow >= 0) {
        /* If clicking on the same piece, deselect it */
        if (g_guiState.selectedRow == row && g_guiState.selectedCol == col) {
            g_guiState.selectedRow = -1;
            g_guiState.selectedCol = -1;
            g_guiState.validMoveCount = 0;
            return;
        }
        
        /* Try to move the selected piece to the destination */
        if (IsValidMove(g_pActiveBoard, g_guiState.selectedRow, g_guiState.selectedCol, 
                        row, col, g_currentPlayer)) {
            
            /* Process and log the move using the same logging system as the terminal game */
            char san[16] = "";
            if (GUI_ProcessMove(g_pActiveBoard, g_pLogFile, g_fullTurnCount, g_currentPlayer,
                                g_guiState.selectedRow, g_guiState.selectedCol, row, col,
                                san, sizeof(san))) {

                char moveStr[64];
                if (g_currentPlayer == 'w') {
                    snprintf(moveStr, sizeof(moveStr), "%d. %s ", g_fullTurnCount, san);
                } else {
                    snprintf(moveStr, sizeof(moveStr), "%s\n", san);
                }
                AppendToMoveLog(moveStr);

                AdvanceTurn();
                CheckGUIEndState();

                if (!g_gameOver) {
                    RunAIMoveIfNeeded();
                }
            }
            /* Deselect and clear valid moves */
            g_guiState.selectedRow = -1;
            g_guiState.selectedCol = -1;
            g_guiState.validMoveCount = 0;
            return;
        }
        
        /* If clicking on another piece of the current player, select it instead */
        if (clickedPiece.type != ' ' && clickedPiece.color == g_currentPlayer) {
            g_guiState.selectedRow = row;
            g_guiState.selectedCol = col;
            CalculateValidMoves();
            return;
        }
        
        /* Otherwise, deselect */
        g_guiState.selectedRow = -1;
        g_guiState.selectedCol = -1;
        g_guiState.validMoveCount = 0;
        return;
    }
    
    /* No piece is selected yet - select one */
    if (clickedPiece.type != ' ' && clickedPiece.color == g_currentPlayer) {
        g_guiState.selectedRow = row;
        g_guiState.selectedCol = col;
        CalculateValidMoves();
    }
}

/*
 * GTK event handler for mouse clicks on the board
 */
static gboolean OnBoardClick(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (event->button != 1) {
        return FALSE;
    }

    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    
    double cellWidth = (double)width / COLS;
    double cellHeight = (double)height / ROWS;
    
    int col = (int)(event->x / cellWidth);
    int row = (int)((height - event->y) / cellHeight);  /* Invert for bottom-up */
    
    HandleBoardClick(row, col);
    gtk_widget_queue_draw(widget);  /* Redraw board */
    
    return TRUE;
}

/*
 * Cairo callback triggered via the "draw" signal.
 * Responsible for rendering the 8x10 Anteater Chess grid dynamically
 * based on the current window dimensions, along with piece images.
 */
static gboolean OnDrawBoard(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);

    double cellWidth = (double)width / COLS;
    double cellHeight = (double)height / ROWS;

    /* Draw the chessboard grid */
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if ((row + col) % 2 == 0) {
                cairo_set_source_rgb(cr, 0.93, 0.86, 0.73); 
            }
            else {
                cairo_set_source_rgb(cr, 0.46, 0.59, 0.34); 
            }
            
            /* Invert row index so rank 1 (row 0) renders at the bottom of the window */
            int renderRow = ROWS - 1 - row;
            
            cairo_rectangle(cr, col * cellWidth, renderRow * cellHeight, cellWidth, cellHeight);
            cairo_fill(cr);
        }
    }


    /* Draw all pieces on the board */
    if (g_pActiveBoard != NULL) {
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                RenderPiece(cr, g_pActiveBoard->grid[row][col], row, col, cellWidth, cellHeight);
            }
        }
    }

    /* Highlight selected piece */
    if (g_guiState.selectedRow >= 0) {
        int renderRow = ROWS - 1 - g_guiState.selectedRow;
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.4);  /* Yellow semi-transparent */
        cairo_rectangle(cr, g_guiState.selectedCol * cellWidth, 
                        renderRow * cellHeight, cellWidth, cellHeight);
        cairo_fill(cr);
    }

        /* Highlight valid move destinations */
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);  /* Red */
    for (int i = 0; i < g_guiState.validMoveCount; i++) {
        int moveRow = g_guiState.validMoves[i][0];
        int moveCol = g_guiState.validMoves[i][1];
        int renderRow = ROWS - 1 - moveRow;
        
        double centerX = moveCol * cellWidth + cellWidth / 2;
        double centerY = renderRow * cellHeight + cellHeight / 2;
        double radius = cellWidth / 6;
        
        cairo_arc(cr, centerX, centerY, radius, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    return FALSE;
}

/*
 * Callback for the header bar toggle button.
 * Modifies the GtkRevealer's child property to animate the sidebar in/out.
 */
static void OnSidebarToggle(GtkToggleButton *toggle_button, gpointer user_data)
{
    GtkRevealer *revealer = GTK_REVEALER(user_data);
    gboolean is_active = gtk_toggle_button_get_active(toggle_button);
    gtk_revealer_set_reveal_child(revealer, is_active);
}

/*
 * Public function to set game context for the GUI
 * Called from main.c to sync game state (log file, current player)
 */
void SetGUIGameContext(FILE* logFile, char startingPlayer, char gameMode, char aiDifficulty, char playerColor)
{
    g_pLogFile = logFile;
    g_currentPlayer = startingPlayer;
    g_fullTurnCount = 1;
    g_gameMode = gameMode;
    g_aiDifficulty = aiDifficulty;
    g_humanColor = playerColor;
    g_gameOver = 0;

    g_guiState.selectedRow = -1;
    g_guiState.selectedCol = -1;
    g_guiState.validMoveCount = 0;
}

void StartGUI(int argc, char *argv[], Board* pBoard)
{
    g_pActiveBoard = pBoard;
    
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    /* Increased default width to accommodate the 250px sidebar alongside the board */
    gtk_window_set_default_size(GTK_WINDOW(window), 1050, 640);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* * Implement a GtkHeaderBar to replace the native window title bar.
     * This provides a clean location for the sidebar toggle control.
     */
    GtkWidget *headerBar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerBar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(headerBar), "Anteater Chess");
    gtk_window_set_titlebar(GTK_WINDOW(window), headerBar);

    GtkWidget *toggleBtn = gtk_toggle_button_new_with_label("Move Log");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(headerBar), toggleBtn);

    /* Main container: Horizontal box to place sidebar and board side-by-side */
    GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), mainBox);

    /* * GtkRevealer handles the smooth slide animation for the collapsible UI component.
     */
    GtkWidget *revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 300);
    
    /* Pack revealer with FALSE expand/fill so it strictly respects its requested size */
    gtk_box_pack_start(GTK_BOX(mainBox), revealer, FALSE, FALSE, 0);

    g_signal_connect(toggleBtn, "toggled", G_CALLBACK(OnSidebarToggle), revealer);

    /* Scrolled window to handle future log overflow */
    GtkWidget *scrolledWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolledWin, 250, -1);
    gtk_container_add(GTK_CONTAINER(revealer), scrolledWin);

    /* Uneditable text view acting as the log container */
    GtkWidget *logTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(logTextView), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(logTextView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(logTextView), GTK_WRAP_WORD);
    
    /* Apply a slight margin for UX text readability */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(logTextView), 5);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(logTextView), 5);
    gtk_container_add(GTK_CONTAINER(scrolledWin), logTextView);
    
    /* Store reference to the move log for gameplay updates */
    g_pLogTextView = logTextView;

    /* Core rendering area with fixed aspect ratio */
    GtkWidget *drawingArea = gtk_drawing_area_new();
    g_pBoardArea = drawingArea;
    g_signal_connect(drawingArea, "draw", G_CALLBACK(OnDrawBoard), NULL);
    g_signal_connect(drawingArea, "button-press-event", G_CALLBACK(OnBoardClick), NULL);
    gtk_widget_add_events(drawingArea, GDK_BUTTON_PRESS_MASK);
    
    /* Wrap the drawing area in an aspect frame to maintain board proportions
     * The board is 10 columns x 8 rows, so the aspect ratio is 10:8 or 5:4
     * This ensures each cell is square and the board doesn't distort on resize
     */
    GtkWidget *aspectFrame = gtk_aspect_frame_new(NULL, 0.5, 0.5, (float)COLS / ROWS, FALSE);
    gtk_container_add(GTK_CONTAINER(aspectFrame), drawingArea);
    
    /* Pack the aspect frame with TRUE expand to consume all remaining window space */
    gtk_box_pack_start(GTK_BOX(mainBox), aspectFrame, TRUE, TRUE, 0);

    /* Initialize the application state with the sidebar open */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggleBtn), TRUE);

    gtk_widget_show_all(window);
    if (g_gameMode == '2' && g_humanColor == 'b') {
        RunAIMoveIfNeeded();
        gtk_widget_queue_draw(drawingArea);
    }
    gtk_main();
}
