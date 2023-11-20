#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <time.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include <resources/raygui.h>

// struct definitions
typedef enum State
{
    MENU,
    GAME,
    SETTING,
    GAMEOVER,
    PAUSE,
    NONE
} State;

typedef enum Gamemode
{
    LOCAL,
    AI_MINIMAX,
    AI_ML
} Gamemode;

typedef enum DifficultyMode
{
    EASY,
    MEDIUM,
    HARD
} DifficultyMode;

typedef enum Player_Type
{
    PLAYER_HUMAN,
    PLAYER_AI,
    PLAYER_NONE
} Player_Type;

typedef enum Tile
{
    EMPTY,
    CROSS,
    CIRCLE
} Tile;

typedef enum Data_Result
{
    NEGATIVE,
    POSITIVE
} Data_Result;

typedef struct Move
{
    int row;
    int column;
} Move;

typedef struct Player
{
    Player_Type type;
    Tile tile;
} Player;

typedef struct ML_Data_Row
{
    Tile tile[9];
    Data_Result result;
} ML_Data_Row;

typedef struct Predicted_Result
{
    Data_Result result;
    double score;
} Predicted_Result;

typedef struct Confusion_Matrix
{
    double true_positive;
    double false_positive;
    double true_negative;
    double false_negative;
    double probability_error;
    double accuracy;
} Confusion_Matrix;

// definitions for UI
#define UI_OFFSET 60
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 100
#define COLUMN 3
#define ROW 3
#define GAME_END_DELAY 5

// definitions for ML
#define MAX_DATASET_SIZE 958
#define MAX_DATAROW_SIZE 28
#define TRAINING_DATA_WEIGHT 0.8
#define NB_DATASET_FILE "resources/tictactoe.txt"

// function prototypes for game logic
void init();
void start_game();
void update_game_render();
void update_menu();
void update_game();
void update_setting();
void update_pause();
void update_gameover();
void set_current_state(State state);
void handle_mouse_input();
void change_player_turn();

// function prototypes for grid logic
void render_grid();
void render_tile(int x_coord, int y_coord, Tile tile);
void render_text_ui();
void render_line(Move start, Move end, float thickness);
void populate_grid(Tile tile);
bool set_tile(int row, int col, Tile tile);
bool is_tile_placeable(int row, int col);

// function prototypes for win condition logic
int is_board_full();
int evaluate();
bool check_win_condition();

// function prototypes for minimax logic
int mini_max(int depth, int is_max, int max_depth, int alpha, int beta);
Move get_mini_max_best_move();

// function prototypes for ML logic
void read_ml_dataset(char file_name[]);
void shuffle_dataset();
void naive_bayes_learn(float training_data_weight);
ML_Data_Row get_current_grid();
Predicted_Result naive_bayes_predict(ML_Data_Row data_row);
Move get_naive_bayes_best_move();
Confusion_Matrix calculate_confusion_matrix();

// global constants for UI
const int CELL_WIDTH = SCREEN_WIDTH / COLUMN;
const int CELL_HEIGHT = SCREEN_HEIGHT / ROW;
const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;
const float WIN_LINE_THICKNESS = CELL_WIDTH / 4;

// global variables for game logic
Texture2D g_cross_circle_texture;
clock_t g_start_time, g_elapsed_time;
Tile g_grid[ROW][COLUMN];
Player g_player_one, g_player_two;
Player *gp_current_player;
Player *gp_winner;
Move g_winner_start, g_winner_end;
Gamemode g_current_gamemode;
DifficultyMode g_game_difficulty_mode;
State g_previous_state = NONE, g_current_state = MENU;

// global variables for ML logic
ML_Data_Row g_dataset_array[MAX_DATASET_SIZE];
int g_dataset_count = 0;
double g_naive_bayes_probability[9][6];
double g_positive_counter = 0, g_negative_counter = 0;
Confusion_Matrix g_current_confusion_matrix;

// current grid design, row = 3, column = 3
// 0,0 | 0,1 | 0,2
// 1,0 | 1,1 | 1,2
// 2,0 | 2,1 | 2,2
// index of each grid

/*
Main function of the program
Initiate certain variables and functions that are to be ran one time only
Contains the main loop of the game
*/
int main(void)
{
    // initialize the window and size using raylib's library
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tic Tac Toe");
    // load the texture for the cross and circle
    g_cross_circle_texture = LoadTexture("resources/tictactoe.png");
    SetExitKey(0); // prevent esc from closing the window
    read_ml_dataset(NB_DATASET_FILE);
    GuiLoadStyle("resources/style_candy.rgs");

    // main game loop
    while (!WindowShouldClose())
    {
        // if state changes, means this state is just entered, run init function
        if (g_previous_state != g_current_state)
        {
            init();
            g_previous_state = g_current_state;
        }
        // switch case to run the respective update loop for different game states
        switch (g_current_state)
        {
        case MENU:
            update_menu();
            break;
        case GAME:
            update_game();
            update_game_render();
            break;
        case SETTING:
            update_setting();
            break;
        case GAMEOVER:
            update_gameover();
            break;
        case PAUSE:
            update_pause();
            break;
        default:
            exit(1);
        }
    }

    CloseWindow();
    return 0;
}

/*
init function that runs once when state changes
*/
void init()
{
    switch (g_current_state)
    {
    case MENU:
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        break;
    case GAME:
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT + UI_OFFSET);
        if (g_previous_state != PAUSE)
            start_game();
        break;
    case SETTING:
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        break;
    case GAMEOVER:
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        break;
    case PAUSE:
        SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        break;
    default:
        exit(1);
    }
}

/*
start a fresh game of tic tac toe function
*/
void start_game()
{
    // set all the grids to be empty
    populate_grid(EMPTY);
    // if the currenmt gamemode is local, set player one and two to be human
    if (g_current_gamemode == LOCAL)
    {
        g_player_one = (Player){PLAYER_HUMAN, CIRCLE};
        g_player_two = (Player){PLAYER_HUMAN, CROSS};
    }
    // else if the current gamemode is minimax, set player one to be human and player two to be AI
    else if (g_current_gamemode == AI_MINIMAX)
    {
        g_player_one = (Player){PLAYER_HUMAN, CIRCLE};
        g_player_two = (Player){PLAYER_AI, CROSS};
    }
    // else if the current gamemode is machine learning, init relevant functions and set player one to be human and player two to be AI
    else if (g_current_gamemode == AI_ML)
    {
        shuffle_dataset();
        naive_bayes_learn(TRAINING_DATA_WEIGHT);
        g_current_confusion_matrix = calculate_confusion_matrix();
        g_player_one = (Player){PLAYER_HUMAN, CIRCLE};
        g_player_two = (Player){PLAYER_AI, CROSS};
    }

    // set the starting player to be player one and clear the winner
    gp_current_player = &g_player_one;
    gp_winner = NULL;
}

/*
Render loop for all render related functions for the game
*/
void update_game_render()
{
    // start drawing buffer
    BeginDrawing();
    // clear screen and set white
    ClearBackground((Color){255, 245, 225, 255});
    // render the grid of the game including the tiles
    render_grid();
    // render the text ui during the game
    render_text_ui();
    // end drawing buffer
    EndDrawing();
}

/*
Update loop for the main menu
*/
void update_menu()
{
    // begin drawing buffer
    BeginDrawing();
    ClearBackground((Color){255, 245, 225, 255});

    const char *MENU_TITLE = "Tic Tac Toe";

    // draw the text for the title of the game
    DrawText(MENU_TITLE, HALF_SCREEN_WIDTH - MeasureText(MENU_TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, (Color){117, 64, 53, 255});
    // draw  the start button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Start Game"))
    {
        set_current_state(GAME);
    }
    // draw the setting button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Settings"))
    {
        set_current_state(SETTING);
    }

    // draw the quit button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Quit"))
        CloseWindow();

    // end drawing buffer
    EndDrawing();
}

/*
main game logic update loop
*/
void update_game()
{
    // if the escape button is pressed, set current state to pause
    if (IsKeyReleased(KEY_ESCAPE))
    {
        set_current_state(PAUSE);
        return;
    }

    // stop updating game loop if winner is found or board is full
    if (gp_winner != NULL || is_board_full())
    {
        return;
    }

    // different game logic for different game mode states
    switch (g_current_gamemode)
    {
    case LOCAL:
        // receive user input and place tile
        handle_mouse_input();
        break;
    case AI_MINIMAX:
        // receive user input and place tile
        if (gp_current_player == &g_player_one)
        {
            handle_mouse_input();
        }
        else if (gp_current_player == &g_player_two)
        {
            // get the best move from minimax algo and then set the tile and change player turn
            Move best_move = get_mini_max_best_move();
            set_tile(best_move.row, best_move.column, g_player_two.tile);
            change_player_turn();
        }
        break;
    case AI_ML:
        // receive user input and place tile
        if (gp_current_player == &g_player_one)
        {
            handle_mouse_input();
        }
        else if (gp_current_player == &g_player_two)
        {
            // get the best move from naive bayes algo and then set tile and change player turn
            Move best_move = get_naive_bayes_best_move();
            set_tile(best_move.row, best_move.column, g_player_two.tile);
            change_player_turn();
        }
        break;
    }
}

/*
update setting loop
*/
void update_setting()
{
    // begin drawing buffer
    BeginDrawing();
    // clear the background
    ClearBackground((Color){255, 245, 225, 255});

    // if the escape button is pressed while in the settings, return back to main menu
    if (IsKeyReleased(KEY_ESCAPE))
    {
        set_current_state(MENU);
        return;
    }

    const char *TITLE = "Setting";

    // draw the text of the settings
    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, (Color){117, 64, 53, 255});
    // drawing the gui box for the different gamemodes
    GuiComboBox((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Local;Mini Max AI;Machine Learning", (int *)&g_current_gamemode);

    // if current gamemode is minimax, show difficulty setting, else if current gamemode is ML, show confusion matrix as button 2
    if (g_current_gamemode == AI_MINIMAX)
    {
        GuiComboBox((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Easy;Medium;Hard", (int *)&g_game_difficulty_mode);
    }
    // if the gamemode is ML, we show the confusion matrix
    else if (g_current_gamemode == AI_ML)
    {
        // Draw confusion matrix as a table, TP(True Positive), FP(False Positive), TN(True Negative), FN(False Negative)
        GuiGroupBox((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Confusion Matrix");
        DrawText(TextFormat("TP: %g", g_current_confusion_matrix.true_positive), HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2 + 10, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2 + 10, 20, (Color){117, 64, 53, 255});
        DrawText(TextFormat("FP: %g", g_current_confusion_matrix.false_positive), HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2 + 10, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2 + 30, 20, (Color){117, 64, 53, 255});
        DrawText(TextFormat("TN: %g", g_current_confusion_matrix.true_negative), HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2 + 10, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2 + 50, 20, (Color){117, 64, 53, 255});
        DrawText(TextFormat("FN: %g", g_current_confusion_matrix.false_negative), HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2 + 10, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2 + 70, 20, (Color){117, 64, 53, 255});
    }

    // print the return to main menu button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        set_current_state(MENU);
    EndDrawing();
}

/*
pause screen update loop
*/
void update_pause()
{
    // begin drawing buffer and set white background
    BeginDrawing();
    ClearBackground((Color){255, 245, 225, 255});

    const char *TITLE = "Paused";

    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, (Color){117, 64, 53, 255});
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Resume"))
        set_current_state(GAME);
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        set_current_state(GAME);
        start_game();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        set_current_state(MENU);

    // clear drawing buffer
    EndDrawing();
}

/*
gameover screen udpate loop
*/
void update_gameover()
{
    // begin drawing buffer and set white background
    BeginDrawing();
    ClearBackground((Color){255, 245, 225, 255});

    char *TITLE = "Player 1 Wins!";

    // check if there is a winner and set the title accordingly
    if (gp_winner == &g_player_two && g_player_two.type == PLAYER_AI)
    {
        TITLE = "AI wins!";
    }
    else if (gp_winner == &g_player_two)
    {
        TITLE = "Player 2 Wins!";
    }
    else if (is_board_full() && gp_winner == NULL)
    {
        TITLE = "Draw!";
    }

    // draw the title text for gameover
    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, (Color){117, 64, 53, 255});

    // draw the restart button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        set_current_state(GAME);
    }
    // draw the return to main menu button
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        set_current_state(MENU);

    EndDrawing();
}

/*
function to set the current state of the game
*/
void set_current_state(State state)
{
    if (g_current_state == state)
        return;

    g_previous_state = g_current_state;
    g_current_state = state;
}

/*
function to handle mouse input from user
*/
void handle_mouse_input()
{
    // fail safe check, if game is over player should not be able to input anything
    if (gp_winner != NULL || is_board_full())
        return;

    // if left click is pressed, get the position and try to place a tile
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse_position = GetMousePosition();

        // mouse click ignore if its in the empty space above the grid
        if (mouse_position.y < UI_OFFSET)
            return;

        // int division to get the row and column of the grid
        int col = mouse_position.x / CELL_WIDTH;
        int row = (mouse_position.y - UI_OFFSET) / CELL_HEIGHT;
        // try to set the tile, and if it is successful change player turn
        if (set_tile(row, col, gp_current_player->tile))
        {
            change_player_turn();
        }
    }
}

/*
Change the current player turn
*/
void change_player_turn()
{
    // if current player is player one, change to player two, vice versa
    if (gp_current_player == &g_player_one)
        gp_current_player = &g_player_two;
    else if (gp_current_player == &g_player_two)
        gp_current_player = &g_player_one;
}

// main grid rendering function
void render_grid()
{
    // nested loop to go through every cell in the grid
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            // calculate the coordinates for each tile
            int x_coord = j * CELL_WIDTH;
            int y_coord = i * CELL_HEIGHT + UI_OFFSET;
            // draw rectangles for each cell
            DrawRectangleLines(x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT, (Color){117, 64, 53, 255});
            // we render each tile
            render_tile(x_coord, y_coord, g_grid[i][j]);
        }

    // if the game is over, we draw the line and also show a countdown
    if (gp_winner != NULL || is_board_full())
    {
        // get the current elapsed time
        g_elapsed_time = (double)(clock() - g_start_time) / CLOCKS_PER_SEC;
        // if elapsed time has crossed more than the delay, we change the state to gameover
        if (g_elapsed_time >= GAME_END_DELAY + g_start_time)
            set_current_state(GAMEOVER);
        // draw the countdown timer
        const char *countdown_timer = TextFormat("Game will end in %.1f", (float)((GAME_END_DELAY + g_start_time) - g_elapsed_time));
        DrawText(countdown_timer, SCREEN_WIDTH / 2 - MeasureText(countdown_timer, 50) / 2, SCREEN_HEIGHT / 2 + UI_OFFSET, 50, (Color){117, 64, 53, 255});
        // if board is full and no winner, dont draw line, however if board is full but winner is found, draw line, else draw line if winner is found
        if (!is_board_full() || gp_winner != NULL)
            render_line(g_winner_start, g_winner_end, WIN_LINE_THICKNESS);
    }
}

/*
Function to render tile individual tiles
*/
void render_tile(int x_coord, int y_coord, Tile tile)
{
    // create a rectangle variable for where we want our tile to be
    Rectangle destination = {x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT};
    Rectangle source;

    // switch case to render the tile according to the tile type
    switch (tile)
    {
    case CROSS:
        source = (Rectangle){0, 0, 100, 100};
        DrawTexturePro(g_cross_circle_texture, source, destination, (Vector2){0, 0}, 0, WHITE);
        break;
    case CIRCLE:
        source = (Rectangle){100, 0, 100, 100};
        DrawTexturePro(g_cross_circle_texture, source, destination, (Vector2){0, 0}, 0, WHITE);
        break;
    case EMPTY:
        break;
    }
}

/*
function to render the text ui at the top of game screen
*/
void render_text_ui()
{
    // get the coord at half of the screen
    int x_coord = SCREEN_WIDTH / 2;
    // get the coord at 1/6 of the blank area
    int y_coord = UI_OFFSET / 6;
    int font_size = 40;
    char top_text[30] = "-";

    // if the game is over, print respective title
    if (gp_winner != NULL || is_board_full())
    {
        if (gp_winner == &g_player_one)
            strcpy(top_text, "Player 1 wins!");
        else if (gp_winner == &g_player_two)
            strcpy(top_text, "Player 2 wins!");
        else if (is_board_full())
            strcpy(top_text, "Draw!");
    }
    else // if game is still going on print the current player turn
    {
        if (gp_current_player == &g_player_one)
            strcpy(top_text, "Player 1's turn");
        else if (gp_current_player)
            strcpy(top_text, "Player 2's turn");
    }

    // draw the text
    DrawText(top_text, x_coord - MeasureText(top_text, font_size) / 2, y_coord, font_size, (Color){117, 64, 53, 255});
}

/*
Function to render the game over line using a start and end position and a thickness
*/
void render_line(Move start, Move end, float thickness)
{
    Vector2 start_position = {start.column * CELL_WIDTH + CELL_WIDTH / 2, start.row * CELL_HEIGHT + CELL_HEIGHT / 2 + UI_OFFSET};
    Vector2 end_position = {end.column * CELL_WIDTH + CELL_WIDTH / 2, end.row * CELL_HEIGHT + CELL_HEIGHT / 2 + UI_OFFSET};
    static const Color red_translucent = (Color){255, 0, 0, 150};
    DrawLineEx(start_position, end_position, thickness, red_translucent);
}

/*
populate the grid according to tile input
*/
void populate_grid(Tile tile)
{
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
            g_grid[i][j] = tile;
}

/*
Sets the desired tile at the desired row and column
Use this function to set the tile instead of directly accessing the g_grid array
*/
bool set_tile(int row, int col, Tile tile)
{
    if (is_tile_placeable(row, col))
    {
        g_grid[row][col] = tile;
        if (check_win_condition())
            g_start_time = (double)clock() / CLOCKS_PER_SEC;
        return true;
    }
    else
        return false;
}

/*
Checks if the desired tile is placeable
*/
bool is_tile_placeable(int row, int col)
{
    if (g_grid[row][col] == EMPTY)
        return true;
    else
        return false;
}

/*
Returns 1 if the board is full, else return 0
*/
int is_board_full()
{
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            if (g_grid[i][j] == EMPTY)
            {
                return 0;
            }
        }
    }
    return 1;
}

/*
Evaluate the current board and return 1 if g_player_two has won, -1 if g_player_one has won.
Reset the gp_winner pointer to NULL after evaluation. Mainly used for minimax algorithm.
*/
int evaluate()
{
    check_win_condition();

    // if g_player_two has won, return 1, reset gp_winner to NULL
    if (gp_winner == &g_player_two)
    {
        gp_winner = NULL;
        return 1;
    }
    else if (gp_winner == &g_player_one)
    {
        gp_winner = NULL;
        return -1;
    }

    return 0;
}

/*
Returns a boolean value if the current grid is a winning condition and updates gp_winner
Variables g_winner_start and g_winner_end are the start and end position of the winning line
It returns a boolean if there is a win, but also updates the gp_winner pointer to the winner
*/
bool check_win_condition()
{
    Tile temp;
    // loop through each row, and compare columns
    // algo to check each row
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            if (j == 0)
            {
                temp = g_grid[i][j];
                g_winner_start = (Move){i, j};
                continue;
            }
            else if (temp != g_grid[i][j])
            {
                temp = EMPTY;
                break;
            }
            else if (j == COLUMN - 1)
            {
                if (g_player_one.tile == temp)
                {
                    gp_winner = &g_player_one;
                    g_winner_end = (Move){i, j};
                    return true;
                }
                else if (g_player_two.tile == temp)
                {
                    gp_winner = &g_player_two;
                    g_winner_end = (Move){i, j};
                    return true;
                }
            }
        }

    // loop through each column, and compare rows
    // algo to check each column
    for (int i = 0; i < COLUMN; i++)
        for (int j = 0; j < ROW; j++)
        {
            if (j == 0)
            {
                temp = g_grid[j][i];
                g_winner_start = (Move){j, i};
                continue;
            }
            else if (temp != g_grid[j][i])
            {
                temp = EMPTY;
                break;
            }
            else if (j == ROW - 1)
            {
                if (g_player_one.tile == temp)
                {
                    gp_winner = &g_player_one;
                    g_winner_end = (Move){j, i};
                    return true;
                }
                else if (g_player_two.tile == temp)
                {
                    gp_winner = &g_player_two;
                    g_winner_end = (Move){j, i};
                    return true;
                }
            }
        }

    // algo to check top left to bottom right diagonal
    for (int i = 0; i < ROW; i++)
    {
        if (i == 0)
        {
            temp = g_grid[i][i];
            g_winner_start = (Move){i, i};
            continue;
        }
        else if (temp != g_grid[i][i])
        {
            temp = EMPTY;
            break;
        }
        else if (i == ROW - 1)
        {
            if (g_player_one.tile == temp)
            {
                gp_winner = &g_player_one;
                g_winner_end = (Move){i, i};
                return true;
            }
            else if (g_player_two.tile == temp)
            {
                gp_winner = &g_player_two;
                g_winner_end = (Move){i, i};
                return true;
            }
        }
    }

    // algo to check bottom left to top right diagonal
    for (int i = 0; i < ROW; i++)
    {
        if (i == 0)
        {
            temp = g_grid[ROW - 1 - i][i];
            g_winner_start = (Move){ROW - 1 - i, i};
            continue;
        }
        else if (temp != g_grid[ROW - 1 - i][i])
        {
            temp = EMPTY;
            break;
        }
        else if (i == ROW - 1)
        {
            if (g_player_one.tile == temp)
            {
                gp_winner = &g_player_one;
                g_winner_end = (Move){ROW - 1 - i, i};
                return true;
            }
            else if (g_player_two.tile == temp)
            {
                gp_winner = &g_player_two;
                g_winner_end = (Move){ROW - 1 - i, i};
                return true;
            }
        }
    }

    return false;
}

/*
Recursive function that calculates the minimax value of the current board state,
implemented with alpha-beta pruning.
*/
int mini_max(int depth, int is_max, int max_depth, int alpha, int beta)
{
    int score = evaluate();

    if (score != 0)
        return score;
    if (is_board_full() || depth == max_depth)
        return 0;

    // initalize a best value base on the current player (max or min)
    int best_val = is_max ? -1000 : 1000;

    // loop through the board cells
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            if (g_grid[i][j] == EMPTY)
            {
                // temporarily set the cell with the current player's tile
                g_grid[i][j] = is_max ? g_player_two.tile : g_player_one.tile;

                // recursively calculate the minimax value
                int move_val = mini_max(depth + 1, !is_max, max_depth, alpha, beta);

                // update the best value based on the current player (max or min)
                best_val = is_max ? fmax(best_val, move_val) : fmin(best_val, move_val);

                // undo the move (backtrack)
                g_grid[i][j] = EMPTY;

                // alpha-beta pruning codes
                if (is_max)
                {
                    alpha = fmax(alpha, move_val);
                }
                else
                {
                    beta = fmin(beta, move_val);
                }

                /*
                if beta is less than or equal to alpha, break out of the loop
                as the rest of the moves will not be considered
                */
                if (beta <= alpha)
                {
                    break;
                }
            }
        }
    }

    return best_val;
}

/*
Returns the best move for the minimax player using the minimax algorithm
*/
Move get_mini_max_best_move()
{
    // set initial difficult of miniMax mod to easy to look only 1 move ahead
    int difficulty = 0;

    // if difficulty is medium or hard update difficulty
    if (g_game_difficulty_mode == MEDIUM)
        difficulty = 1;
    else if (g_game_difficulty_mode == HARD)
        difficulty = (ROW * COLUMN) - 1; // look ahead all the way to the end

    // initial best value to a very low value
    int best_val = -1000;

    // initialize best move row and column value
    Move best_move = {-1, -1};

    // loop through the board cells
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            // if cell is empty, attempt move and see if it is the best move
            if (g_grid[i][j] == EMPTY)
            {
                g_grid[i][j] = g_player_two.tile;
                int move_val = mini_max(0, 0, difficulty, -1000, 1000);
                g_grid[i][j] = EMPTY;
                // if move_val is better than best_val, update best_val and best_move_row and best_move_column
                if (move_val > best_val)
                {
                    best_move.row = i;
                    best_move.column = j;
                    best_val = move_val;
                }
            }
        }
    }

    // return the best move
    return best_move;
}

/*
Takes in a file name and read the dataset into g_dataset_array
*/
void read_ml_dataset(char file_name[])
{
    // attempt to open file for reading
    FILE *dataset_file = fopen(file_name, "r");

    // error checking for file opening
    if (!dataset_file)
    {
        printf("Error opening file %s\n", file_name);
        exit(1);
    }

    char line[MAX_DATAROW_SIZE];
    // read from data in lines into the struct
    while (g_dataset_count < MAX_DATASET_SIZE && fgets(line, sizeof(line), dataset_file))
    {
        // remove the newline character from the end of each line
        line[strcspn(line, "\n")] = '\0';

        // go through each character in the line and assign respective tile to the struct
        for (int i = 0; i < 17; i += 2) // skips the comma
        {
            if (line[i] == 'x')
                g_dataset_array[g_dataset_count].tile[i / 2] = CROSS;
            else if (line[i] == 'o')
                g_dataset_array[g_dataset_count].tile[i / 2] = CIRCLE;
            else if (line[i] == 'b')
                g_dataset_array[g_dataset_count].tile[i / 2] = EMPTY;
        }

        // reverse search line and get the last token which is either POSTIIVE or NEGATIVE
        char *token = strrchr(line, ',') + 1;

        // set the current row result to the token value positive or negative
        g_dataset_array[g_dataset_count].result = strcmp(token, "positive") == 0 ? POSITIVE : NEGATIVE;

        // g_dataset_count is the total number of lines in dataset
        g_dataset_count++;
    }

    fclose(dataset_file);
}

/*
shuffle the dataset array using fisher yates algorithm shuffle
*/
void shuffle_dataset()
{
    // fisher yates shuffle
    // initialize random seed so that the shuffle is different each time
    srand(time(NULL));

    // loop through the dataset array and swap the current index with a random index
    for (int i = g_dataset_count - 1; i > 0; i--)
    {
        int j;

        // keep generating random index until it is not the same as i
        do
        {
            j = rand() % (i + 1);
        } while (i == j);

        // swap the current index with the random index
        ML_Data_Row temp_data_row = g_dataset_array[i];
        g_dataset_array[i] = g_dataset_array[j];
        g_dataset_array[j] = temp_data_row;
    }
}

/*
Train the naive bayes probability algorithm and store the probability in g_naive_bayes_probability
*/
void naive_bayes_learn(float training_data_weight)
{
    // reset counters and probability array
    g_positive_counter = 0;
    g_negative_counter = 0;
    memset(g_naive_bayes_probability, 0, sizeof(g_naive_bayes_probability));

    // only use a portion of the total dataset for learning
    int training_data_count = ceil(g_dataset_count * training_data_weight);

    // loop through the training data and calculate the probability of each tile
    for (int i = 0; i < training_data_count; i++)
    {
        // get the current training data
        ML_Data_Row current_row = g_dataset_array[i];

        // increment the positive and negative counter
        if (current_row.result == POSITIVE)
        {
            g_positive_counter++;
        }
        else if (current_row.result == NEGATIVE)
        {
            g_negative_counter++;
        }

        /*
        increment the count of each tile in g_naive_bayes_probability, where
        for each row, first 3 columns are positive {Xp, Op, Bp}, last 3 columns are negative {Xn, On, Bn}
        row_offset will offset the array index depending whether the current result is positive or negative
        */
        int row_offset = current_row.result == POSITIVE ? 0 : 3;

        for (int row = 0; row < 9; row++)
        {
            switch (current_row.tile[row])
            {
            case CROSS:
                g_naive_bayes_probability[row][0 + row_offset]++;
                break;
            case CIRCLE:
                g_naive_bayes_probability[row][1 + row_offset]++;
                break;
            case EMPTY:
                g_naive_bayes_probability[row][2 + row_offset]++;
                break;
            }
        }
    }

    /*
    calculate the probability of each tile by taking the total
    occurence of state of the cell / total occurence of positive or negative
    */
    for (int row = 0; row < 9; row++)
    {
        for (int col = 0; col < 6; col++)
        {
            // first 3 columns are positive {Xp, Op, Bp}, last 3 columns are negative {Xn, On, Bn}
            g_naive_bayes_probability[row][col] /= col < 3 ? g_positive_counter : g_negative_counter;
        }
    }

    // we reuse the counters to store prior probability of positive p(P) and negative p(N)
    g_positive_counter /= training_data_count;
    g_negative_counter /= training_data_count;
}

/*
converts the current grid into a ML_Data_Row struct
*/
ML_Data_Row get_current_grid()
{
    // initialize the current row as a ML_Data_Row struct
    ML_Data_Row current_row;

    // loop through the grid and convert the 2d array into a 1d array
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            current_row.tile[i * 3 + j] = g_grid[i][j];
        }
    }

    return current_row;
}

/*
Takes in a ML_Data_Row struct and returns the predicted result of the function parameter data
*/
Predicted_Result naive_bayes_predict(ML_Data_Row data)
{
    // initialize prior probablity p(P) and p(N)
    double positive_probability = g_positive_counter;
    double negative_probability = g_negative_counter;
    Predicted_Result predicted_result;

    // loop through each tile and multiply the probability into the prior probability
    // column 0 - X positive
    // column 1 - O positive
    // column 2 - B positive
    // column 3 - X negative
    // column 4 - O negative
    // column 5 - B negative
    for (int i = 0; i < 9; i++)
    {
        switch (data.tile[i])
        {
        case CROSS:
            positive_probability *= g_naive_bayes_probability[i][0];
            negative_probability *= g_naive_bayes_probability[i][3];
            break;
        case CIRCLE:
            positive_probability *= g_naive_bayes_probability[i][1];
            negative_probability *= g_naive_bayes_probability[i][4];
            break;
        case EMPTY:
            positive_probability *= g_naive_bayes_probability[i][2];
            negative_probability *= g_naive_bayes_probability[i][5];
            break;
        }
    }

    // compare whether the positive or negative is higher, the higher of those will be the predicted probability
    if (fmax(positive_probability, negative_probability) == positive_probability)
    {
        predicted_result.result = POSITIVE;
        predicted_result.score = positive_probability;
    }
    else
    {
        predicted_result.result = NEGATIVE;
        predicted_result.score = negative_probability;
    }

    return predicted_result;
}

/*
Returns the best move based on the naive bayes prediction
*/
Move get_naive_bayes_best_move()
{
    // initialize best score to a very low value
    double best_score = -1000;
    bool positive_move_found = false;
    Move best_move = {-1, -1};

    // loop through the grid, if cell is empty, place tile and calculate the score
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            // if cell is empty, attempt move and see if it is the best move
            if (g_grid[i][j] == EMPTY)
            {
                // temporarily set the cell with the current player's tile
                g_grid[i][j] = g_player_two.tile;
                // Get the predicted result of the current grid
                Predicted_Result predicted_result = naive_bayes_predict(get_current_grid());
                // undo the move (backtrack)
                g_grid[i][j] = EMPTY;

                // Get the best move by comparing the score of each move, with positive prediction move having higher priority
                if (predicted_result.result == POSITIVE || (predicted_result.result == NEGATIVE && !positive_move_found))
                {
                    if (predicted_result.result == POSITIVE)
                        // if positive move is found, set positive_move_found to true, so that we will only take positive move
                        positive_move_found = true;

                    if (predicted_result.score > best_score)
                    {
                        // if the score is better than the current best score, update the best score and best move
                        best_score = predicted_result.score;
                        best_move.row = i;
                        best_move.column = j;
                    }
                }
            }
        }
    }

    return best_move;
}

/*
Returns the confusion matrix of the current dataset {TP, FP, TN, FN, probability_error, accuracy}
*/
Confusion_Matrix calculate_confusion_matrix()
{
    int data_count = floor(g_dataset_count * (1 - TRAINING_DATA_WEIGHT));
    // initialize confusion matrix
    Confusion_Matrix confusion_matrix = {0, 0, 0, 0, 0, 0};

    // loop through the data and predict the result
    for (int i = 0; i < data_count; i++)
    {
        // read the data from the end of the dataset_array up to 1 - TRAINING_DATA_WEIGHT % of the dataset
        ML_Data_Row current_row = g_dataset_array[g_dataset_count - 1 - i];
        Predicted_Result predicted_result = naive_bayes_predict(current_row);

        if (predicted_result.result == current_row.result)
        {
            if (predicted_result.result == POSITIVE)
                // predicted result is positive and actual result is positive
                confusion_matrix.true_positive++;
            else
                // predicted result is negative and actual result is negative
                confusion_matrix.true_negative++;
        }
        else
        {
            if (predicted_result.result == POSITIVE)
                // predicted result is positive and actual result is negative
                confusion_matrix.false_positive++;
            else
                // predicted result is negative and actual result is positive
                confusion_matrix.false_negative++;
        }
    }

    // divide the total count of each result by the total data count to get the probability
    confusion_matrix.true_positive /= data_count;
    confusion_matrix.true_negative /= data_count;
    confusion_matrix.false_positive /= data_count;
    confusion_matrix.false_negative /= data_count;

    // probability error is the sum of false positive and false negative, where the model makes a mistake
    confusion_matrix.probability_error = confusion_matrix.false_positive + confusion_matrix.false_negative;
    // accuracy is calculated as the number of all correct predictions divided by the total number of the datase
    confusion_matrix.accuracy = (confusion_matrix.true_positive + confusion_matrix.true_negative) / (confusion_matrix.true_positive + confusion_matrix.true_negative + confusion_matrix.probability_error);

    return confusion_matrix;
}