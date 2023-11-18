#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <time.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include <lib/raygui.h>

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

typedef enum Data_Result {
    POSITIVE,
    NEGATIVE
} Data_Result;

typedef struct Move {
    int row;
    int column;
} Move;

typedef struct Player {
    Player_Type type;
    Tile tile;
} Player;

typedef struct Data_Row {
    Tile tile[9];
    Data_Result result;
} Data_Row;

typedef struct Predicted_Result {
    Data_Result result;
    double score;
} Predicted_Result;

typedef struct Confusion_Matrix {
    double true_positive;
    double false_positive;
    double true_negative;
    double false_negative;
    double probability_error;
    double accuracy;
} Confusion_Matrix;

// offset is for the extra screenspace at the top during game
#define UI_OFFSET 150
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 100
#define COLUMN 3
#define ROW 3
#define MAX_DATASET_SIZE 958
#define MAX_DATAROW_SIZE 28
#define TRAINING_DATA_WEIGHT 0.8
#define NB_DATASET_FILE "tictactoe_ml/tictactoe.txt"
const int CELL_WIDTH = SCREEN_WIDTH / COLUMN;
const int CELL_HEIGHT = SCREEN_HEIGHT / ROW;

void Init();
void StartGame();
void UpdateGameRender();
void UpdateMenu();
void UpdateSetting();
void UpdateGame();
void UpdatePause();
void SetCurrentState(State state);
void RenderGrid();
void RenderTile(int x, int y, Tile tile);
void RenderTextUI();

bool SetTile(int x, int y, Tile tile);
bool IsTilePlaceable(int x, int y);
void PopulateGrid(Tile tile);
void CheckWinCondition();
void ChangePlayerTurn();
Data_Row get_current_grid();

// ML Related Functions
void read_ml_dataset(char file_name[]);
void shuffle_dataset();
void naive_bayes_learn(float training_data_weight);
Predicted_Result naive_bayes_predict(Data_Row data_row);
Move get_naive_bayes_best_move();
Confusion_Matrix calculate_confusion_matrix();

// global variables
// try not to access grid directly
Tile Grid[ROW][COLUMN];
Player Player_One, Player_Two;
Player *Current_Player;
Player *Winner;
Gamemode Current_Gamemode;
DifficultyMode gameDifficultyMode;
State Previous_State = NONE; 
State Current_State = MENU;
Texture2D cross_circle_texture;

int g_dataset_count = 0;
Data_Row g_dataset_array[MAX_DATASET_SIZE];
double g_naive_bayes_probability[9][6];
double g_positive_counter = 0, g_negative_counter = 0;

// current grid design, row = 3, column = 3
// 0,0 | 0,1 | 0,2
// 1,0 | 1,1 | 1,2
// 2,0 | 2,1 | 2,2
// index of each grid

// zihao testing
void HandleTilePlacement();
void showWinMenu();
int check_board_full();
int mini_max(int depth, int max_depth, int is_max, int alpha, int beta);
void mini_max_make_best_move();

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "tic tac toe");
    cross_circle_texture = LoadTexture("assets/tictactoe.png");
    SetExitKey(0); // prevent esc from closing the window
    read_ml_dataset(NB_DATASET_FILE);

    // main game loop
    while (!WindowShouldClose())
    {
        // if state changes, run the state init
        if (Previous_State != Current_State)
        {
            Init();
            Previous_State = Current_State;
        }

        switch (Current_State)
        {
            case MENU:
                UpdateMenu();
                break;
            case GAME:
                UpdateGame();
                UpdateGameRender();
                break;
            case SETTING:
                UpdateSetting();
                break;
            case GAMEOVER:
                break;
            case PAUSE:
                UpdatePause();
                break;
            default:
                exit(1);
        }
    }

    CloseWindow();
    return 0;
}

void Init()
{
    switch (Current_State)
    {
        case MENU:
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
            break;
        case GAME:
            SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT + UI_OFFSET);
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

// render update loop
// functions related to rendering should reside here
void UpdateGameRender()
{
    if (Winner != NULL || check_board_full())
    {
        showWinMenu();
        return;
    }
    BeginDrawing();
    // clear screen and set white
    ClearBackground(RAYWHITE);
    RenderGrid();
    RenderTextUI();
    EndDrawing();
}

// start a fresh game of tic tac toe function
void StartGame()
{
    PopulateGrid(EMPTY);
    if (Current_Gamemode == LOCAL)
    {
        Player_One = (Player){PLAYER_HUMAN, CIRCLE};
        Player_Two = (Player){PLAYER_HUMAN, CROSS};
    }
    else if (Current_Gamemode == AI_MINIMAX)
    {
        Player_One = (Player){PLAYER_HUMAN, CIRCLE};
        Player_Two = (Player){PLAYER_AI, CROSS};
    }
    else if (Current_Gamemode == AI_ML)
    {
        shuffle_dataset();
        naive_bayes_learn(TRAINING_DATA_WEIGHT);
        calculate_confusion_matrix();
        Player_One = (Player){PLAYER_HUMAN, CIRCLE};
        Player_Two = (Player){PLAYER_AI, CROSS};
    }
    Current_Player = &Player_One;
    Winner = NULL;
}

// game logic update loop
// functions related to rendering should reside here
void UpdateGame()
{
    if (IsKeyReleased(KEY_ESCAPE))
    {
        SetCurrentState(PAUSE);
        return;
    }

    switch (Current_Gamemode)
    {
        case LOCAL:
            // receive user input and place tile
            HandleTilePlacement();
            break;
        case AI_MINIMAX:
            // receive user input and place tile
            if (Current_Player == &Player_One)
            {
            HandleTilePlacement();
            }
            else if (Current_Player == &Player_Two)
            {
                mini_max_make_best_move();
                ChangePlayerTurn();
            }
            break;
        case AI_ML:
            // receive user input and place tile
            if (Current_Player == &Player_One)
            {
                HandleTilePlacement();
            }
            else if (Current_Player == &Player_Two)
            {
                Move best_move = get_naive_bayes_best_move();
                SetTile(best_move.column, best_move.row, Player_Two.tile);
                ChangePlayerTurn();
            }
            break;
    }

    CheckWinCondition();
}

void SetCurrentState(State state)
{
    Previous_State = Current_State;
    Current_State = state;
}

// Function to handle tile placement logic
void HandleTilePlacement()
{
    if (Winner != NULL || check_board_full())
        return;

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse_position = GetMousePosition();

        if (mouse_position.y < UI_OFFSET)
            return;

        int x = mouse_position.x / CELL_WIDTH;
        int y = (mouse_position.y - UI_OFFSET) / CELL_HEIGHT;
        printf("x: %d, y: %d\n", x, y);
        if (SetTile(x, y, Current_Player->tile))
        {
            ChangePlayerTurn();
        }
    }
}

// menu update loop
void UpdateMenu()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
    const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;

    const char *MENU_TITLE = "Tic Tac Toe";

    DrawText(MENU_TITLE, HALF_SCREEN_WIDTH - MeasureText(MENU_TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, GRAY);
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Start Game"))
    {
        SetCurrentState(GAME);
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Settings"))
    {
        SetCurrentState(SETTING);
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Quit"))
        CloseWindow();

    EndDrawing();
}

// setting update loop
void UpdateSetting()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (IsKeyReleased(KEY_ESCAPE))
    {
        SetCurrentState(MENU);
        return;
    }

    const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
    const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;

    const char *TITLE = "Setting";

    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, BLACK);

    GuiComboBox((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Local;Mini Max AI;Machine Learning", (int *)&Current_Gamemode);
    if (Current_Gamemode == AI_MINIMAX)
    {
        GuiComboBox((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Easy;Medium;Hard", (int *)&gameDifficultyMode);
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        SetCurrentState(MENU);
    EndDrawing();
}

// pause update loop
void UpdatePause()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
    const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;

    const char *TITLE = "Paused";

    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, BLACK);
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Resume"))
        SetCurrentState(GAME);
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        SetCurrentState(GAME);
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        SetCurrentState(MENU);

    EndDrawing();
}

// SHOW WIN GAME MENU
void showWinMenu()
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    char *TITLE = "Player 1 Wins!";

    if (Winner == &Player_Two && Player_Two.type == PLAYER_AI)
    {
        TITLE = "AI wins!";
    }
    else if (Winner == &Player_Two)
    {
        TITLE = "Player 2 Wins";
    }
    else if (check_board_full() && Winner == NULL)
    {
        TITLE = "Draw!";
    }

    const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
    const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;

    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, BLACK);

    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        SetCurrentState(GAME);
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        SetCurrentState(MENU);

    EndDrawing();
}

// main grid rendering function
void RenderGrid()
{
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            int x_coord = j * CELL_WIDTH;
            int y_coord = i * CELL_HEIGHT + UI_OFFSET;
            int CELL_HALF_WIDTH = CELL_WIDTH / 2;
            DrawRectangleLines(x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT, BLACK);

            RenderTile(x_coord, y_coord, Grid[i][j]);
            const char *tile_index = TextFormat("[%d, %d]", i, j);
            DrawText(tile_index, x_coord + CELL_HALF_WIDTH - MeasureText(tile_index, 20) / 2, y_coord + CELL_HALF_WIDTH - 10, 20, BLACK);
        }
}

// function to render a specific tile
void RenderTile(int x_coord, int y_coord, Tile tile)
{   
    Rectangle destination = {x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT};
    Rectangle source;

    switch (tile)
    {
    case CROSS:
        source = (Rectangle){0, 0, 100, 100};
        DrawTexturePro(cross_circle_texture, source, destination, (Vector2){0, 0}, 0, GRAY);
        break;
    case CIRCLE:
        source = (Rectangle){100, 0, 100, 100};
        DrawTexturePro(cross_circle_texture, source, destination, (Vector2){0, 0}, 0, GRAY);
        break;
    case EMPTY:
        break;
    }
}

// render function for the title text
void RenderTextUI()
{
    if (Winner != NULL)
    {
        if (Winner == &Player_One)
            DrawText("Player 1 wins!", 10, 10, 20, BLACK);
        else if (Winner == &Player_Two)
            DrawText("Player 2 wins!", 10, 10, 20, BLACK);
    }
    else
    {
        if (Current_Player == &Player_One)
            DrawText("Player 1's turn", 10, 10, 20, BLACK);
        else if (Current_Player)
            DrawText("Player 2's turn", 10, 10, 20, BLACK);
    }

    Vector2 mouse_position = GetMousePosition();
    const char *mouse_position_text = TextFormat("Mouse Position: (%.0f, %.0f)", mouse_position.x, mouse_position.y);
    DrawText(mouse_position_text, 10, 20, 20, BLACK);
}

// setter for tile
// use this instead of accessing array directly
bool SetTile(int x, int y, Tile tile)
{
    if (IsTilePlaceable(x, y))
    {
        Grid[y][x] = tile;
        return true;
    }
    else
        return false;
}

// check if tile is empty and able to place
bool IsTilePlaceable(int x, int y)
{
    if (Grid[y][x] == EMPTY)
        return true;
    else
        return false;
}

// populate the grid according to tile
void PopulateGrid(Tile tile)
{
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
            Grid[i][j] = tile;
}

// function to check win condition
void CheckWinCondition()
{
    Tile temp;

    // loop through each row, and compare columns
    // algo to check each row
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            if (j == 0)
            {
                temp = Grid[i][j];
                continue;
            }
            else if (temp != Grid[i][j])
            {
                temp = EMPTY;
                break;
            }
            else if (j == COLUMN - 1)
            {
                if (Player_One.tile == temp)
                {
                    Winner = &Player_One;
                    return;
                }
                else if (Player_Two.tile == temp)
                {
                    Winner = &Player_Two;
                    return;
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
                temp = Grid[j][i];
                continue;
            }
            else if (temp != Grid[j][i])
            {
                temp = EMPTY;
                break;
            }
            else if (j == ROW - 1)
            {
                if (Player_One.tile == temp)
                {
                    Winner = &Player_One;
                    return;
                }
                else if (Player_Two.tile == temp)
                {
                    Winner = &Player_Two;
                    return;
                }
            }
        }

    // algo to check top left to bottom right diagonal
    for (int i = 0; i < ROW; i++)
    {
        if (i == 0)
        {
            temp = Grid[i][i];
            continue;
        }
        else if (temp != Grid[i][i])
        {
            temp = EMPTY;
            break;
        }
        else if (i == ROW - 1)
        {
            if (Player_One.tile == temp)
            {
                Winner = &Player_One;
                return;
            }
            else if (Player_Two.tile == temp)
            {
                Winner = &Player_Two;
                return;
            }
        }
    }

    // algo to check bottom left to top right diagonal
    for (int i = 0; i < ROW; i++)
    {
        if (i == 0)
        {
            temp = Grid[ROW - 1 - i][i];
            continue;
        }
        else if (temp != Grid[ROW - 1 - i][i])
        {
            temp = EMPTY;
            break;
        }
        else if (i == ROW - 1)
        {
            if (Player_One.tile == temp)
            {
                Winner = &Player_One;
                return;
            }
            else if (Player_Two.tile == temp)
            {
                Winner = &Player_Two;
                return;
            }
        }
    }
}

void ChangePlayerTurn()
{
    if (Current_Player == &Player_One)
        Current_Player = &Player_Two;
    else if (Current_Player == &Player_Two)
        Current_Player = &Player_One;
}

Data_Row get_current_grid()
{
    Data_Row current_row;

    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            current_row.tile[i * 3 + j] = Grid[i][j];
        }
    }

    return current_row;
}

// Functions check if board is full return 1 if full else 0
int check_board_full()
{
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            if (Grid[i][j] == EMPTY)
            {
                return 0;
            }
        }
    }
    return 1;
}

int evaluate(){
    CheckWinCondition();

    if (Winner == &Player_Two) // Check if Player_Two has won, return 1
    {
        Winner = NULL;
        return 1;
    }
    else if(Winner == &Player_One) // Check if Player_One has won, return -1
    {
        Winner = NULL;
        return -1;
    }
    
    return 0;
}

// Minimax algorithm with alpha-beta pruning
int mini_max(int depth, int is_max, int max_depth, int alpha, int beta)
{
    int score = evaluate();

    if (score != 0) return score;
    if (check_board_full() || depth == max_depth) return 0;

    // initalize a best value base on the current player (max or min)
    int best = is_max ? -1000 : 1000;

    // Loop through the board cells
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            if (Grid[i][j] == EMPTY)
            {
                // Temporarily set the cell with the current player's tile
                Grid[i][j] = is_max ? Player_Two.tile : Player_One.tile;

                // Recursively calculate the minimax value
                int move_val = mini_max(depth + 1, !is_max, max_depth, alpha, beta);

                // Update the best value based on the current player (max or min)
                best = is_max ? fmax(best, move_val) : fmin(best, move_val);

                // Undo the move (backtrack)
                Grid[i][j] = EMPTY;
                
                // alpha-beta pruning codes!!!
                if (is_max)
                {
                    alpha = fmax(alpha, move_val);
                }
                else
                {
                    beta = fmin(beta, move_val);
                }

                // Alpha-beta pruning
                if (beta <= alpha)
                {
                    break;  // Prune the branch
                }
            }
        }
    }

    return best;
}

void mini_max_make_best_move()
{   
    // Set initial difficult of miniMax mod to easy to look only 1 move ahead
    int difficulty = 0;

    // if difficulty is medium or hard update difficulty
    if(gameDifficultyMode == MEDIUM)
        difficulty = 1; 
    else if(gameDifficultyMode == HARD)
        difficulty = 8;

    // initial best value to a very low value
    int best_val = -1000;

    // initialize best move row and column value
    int best_move_row = -1;
    int best_move_column = -1;

    // Loop through the board cells
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {
            // if cell is empty, attempt move and see if it is the best move
            if (Grid[i][j] == EMPTY)
            {
                Grid[i][j] = Player_Two.tile;
                int move_val = mini_max(0, 0, difficulty, -1000, 1000);
                Grid[i][j] = EMPTY;
                // if move_val is better than best_val, update best_val and best_move_row and best_move_column
                if (move_val > best_val)
                {
                    best_move_row = i;
                    best_move_column = j;
                    best_val = move_val;
                }
            }
        }
    }

    // make the best move
    Grid[best_move_row][best_move_column] = Player_Two.tile;
}

void read_ml_dataset(char file_name[50])
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
    // Read from data in lines into the struct
    while (g_dataset_count < MAX_DATASET_SIZE && fgets(line, sizeof(line), dataset_file))
    {
        // Remove the newline character from the end of each line
        line[strcspn(line, "\n")] = '\0';

        // go through each character in the line and assign respective tile to the struct
        for (int i = 0; i < 17; i += 2)
        {
            if (line[i] == 'x')
                g_dataset_array[g_dataset_count].tile[i / 2] = CROSS;
            else if (line[i] == 'o')
                g_dataset_array[g_dataset_count].tile[i / 2] = CIRCLE;
            else if (line[i] == 'b')
                g_dataset_array[g_dataset_count].tile[i / 2] = EMPTY;
        }
        // reverse the line and get the last token which is either POSTIIVE or NEGATIVE
        // store the token into the struct
        char *token = strrchr(line, ',') + 1;
        
        g_dataset_array[g_dataset_count].result = strcmp(token, "positive") == 0 ? POSITIVE : NEGATIVE;

        g_dataset_count++;
    }

    printf("Total dataset count: %d\n", g_dataset_count);
}

void shuffle_dataset()
{
    srand(time(NULL));
    
    for (int i = g_dataset_count - 1; i > 0; i--)
    {
        int j;

        do
        {
            j = rand() % (i + 1);
        } while (i == j);

        Data_Row temp_data_row = g_dataset_array[i];
        g_dataset_array[i] = g_dataset_array[j];
        g_dataset_array[j] = temp_data_row;
    }
}

void naive_bayes_learn(float training_data_weight)
{
    // only use a portion of the total dataset for learning
    int training_data_count = ceil(g_dataset_count * training_data_weight);

    // loop through the training data and calculate the probability of each tile
    for (int i = 0; i < training_data_count; i++)
    {   
        // get the current training data
        Data_Row current_row = g_dataset_array[i];

        // increment the positive and negative counter
        if (current_row.result == POSITIVE)
        {
            g_positive_counter++;
        }
        else if (current_row.result == NEGATIVE)
        {
            g_negative_counter++;
        }

        // loop through each tile
        for (int row = 0; row < 9; row++)
        {
            // increment the count of each tile
            // 2d column array stores the count of each 
            // current_row.result * 3 will offset the array index depending whether positive = 0 or negative = 0
            switch (current_row.tile[row])
                {
                    case CROSS:
                        g_naive_bayes_probability[row][0 + current_row.result * 3]++;
                        break;
                    case CIRCLE:
                        g_naive_bayes_probability[row][1 + current_row.result * 3]++;
                        break;
                    case EMPTY:
                        g_naive_bayes_probability[row][2 + current_row.result * 3]++;
                        break;
                }
        }
    }

    // calculate the probability
    // occurence of state of the cell / total occurence of positive or negative
    /*
        g_naive_bayes_probability[i][0] = g_naive_bayes_probability[i][0] / g_positive_counter;
        g_naive_bayes_probability[i][1] = g_naive_bayes_probability[i][1] / g_positive_counter;
        g_naive_bayes_probability[i][2] = g_naive_bayes_probability[i][2] / g_positive_counter;
        g_naive_bayes_probability[i][3] = g_naive_bayes_probability[i][3] / g_negative_counter;
        g_naive_bayes_probability[i][4] = g_naive_bayes_probability[i][4] / g_negative_counter;
        g_naive_bayes_probability[i][5] = g_naive_bayes_probability[i][5] / g_negative_counter;
    */

    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            g_naive_bayes_probability[i][j] /= j < 3 ? g_positive_counter : g_negative_counter;
        }
    }

    // we reuse the counters to store prior probability of positive p(P) and negative p(N)
    g_positive_counter /= training_data_count;
    g_negative_counter /= training_data_count;
}

Predicted_Result naive_bayes_predict(Data_Row data)
{
    // p(P) and p(N)
    double positive_probability = g_positive_counter;
    double negative_probability = g_negative_counter;
    Predicted_Result predicted_result;

    // loop through each tile and multiply the probability into the prior probability
    // 0 - X positive
    // 1 - O positive
    // 2 - B positive
    // 3 - X negative
    // 4 - O negative
    // 5 - B negative
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

    // compare whether the positive or negative is higher, will be the predicted probability
    // if probability is equal, we will jsut take positive
    if (fmax(positive_probability, negative_probability) == positive_probability)
    {
        printf("Positive\n");
        predicted_result.result = POSITIVE;
        predicted_result.score = positive_probability;
    }
    else
    {
        printf("Negative\n");
        predicted_result.result = NEGATIVE;
        predicted_result.score = negative_probability;
    }

    return predicted_result;
}

Move get_naive_bayes_best_move()
{
    double best_score = -1000;
    bool positive_move_found = false;
    Move best_move;

    // we loop through the whole grid, if the grid is empty, we calculate the probability of the grid
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COLUMN; j++)
        {   
            // if cell is empty, attempt move and see if it is the best move
            if (Grid[i][j] == EMPTY)
            {
                Grid[i][j] = Player_Two.tile;
                Predicted_Result predicted_result = naive_bayes_predict(get_current_grid());
                Grid[i][j] = EMPTY;
                printf("predicted result for row: %d, col: %d is %d score:%g \n", i, j, predicted_result.result, predicted_result.score);

                if (predicted_result.result == POSITIVE || (predicted_result.result == NEGATIVE && !positive_move_found))
                {
                    if (predicted_result.result == POSITIVE)
                        positive_move_found = true;

                    if (predicted_result.score > best_score)
                    {
                        best_score = predicted_result.score;
                        best_move.row = i;
                        best_move.column = j;
                    }
                }
            }
        }
    }
    printf("best move row: %d, col: %d\n", best_move.row, best_move.column);
    return best_move;
}

Confusion_Matrix calculate_confusion_matrix()
{
    int data_count = floor(g_dataset_count * (1 - TRAINING_DATA_WEIGHT));
    Confusion_Matrix confusion_matrix = {0, 0, 0, 0, 0, 0};

    for (int i = 0; i < data_count; i++)
    {
        Data_Row current_row = g_dataset_array[g_dataset_count - 1 - i];
        Predicted_Result predicted_result = naive_bayes_predict(current_row);

        if (predicted_result.result == current_row.result)
        {
            if (predicted_result.result == POSITIVE)
                confusion_matrix.true_positive++;
            else
                confusion_matrix.true_negative++;
        }
        else
        {
            if (predicted_result.result == POSITIVE)
                confusion_matrix.false_positive++;
            else
                confusion_matrix.false_negative++;
        }
    }

    confusion_matrix.true_positive /= data_count;
    confusion_matrix.true_negative /= data_count;
    confusion_matrix.false_positive /= data_count;
    confusion_matrix.false_negative /= data_count;

    confusion_matrix.probability_error = confusion_matrix.false_positive + confusion_matrix.false_negative;
    confusion_matrix.accuracy = (confusion_matrix.true_positive + confusion_matrix.true_negative) / (confusion_matrix.true_positive + confusion_matrix.true_negative + confusion_matrix.probability_error);

    printf("probability error: %g\n", confusion_matrix.probability_error);
    printf("accuracy: %g\n", confusion_matrix.accuracy);
    return confusion_matrix;
}