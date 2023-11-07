#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#include <lib/raygui.h>

typedef enum State
{
    MENU,
    GAME,
    SETTING,
    GAMEOVER,
    PAUSE
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

typedef struct Player
{
    Player_Type type;
    Tile tile;
} Player;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define BUTTON_WIDTH 300
#define BUTTON_HEIGHT 100
#define COLUMN 3
#define ROW 3
const int CELL_WIDTH = SCREEN_WIDTH / COLUMN;
const int CELL_HEIGHT = SCREEN_HEIGHT / ROW;

void Init();
void StartGame();
void UpdateGameRender();
void UpdateMenu();
void UpdateSetting();
void UpdateGame();
void UpdatePause();
void RenderGrid();
void RenderTile(int x, int y, Tile tile);
void RenderTextUI();

bool gameOver = false;
bool SetTile(int x, int y, Tile tile);
bool IsTilePlaceable(int x, int y);
void PopulateGrid(Tile tile);
void CheckWinCondition();
void ChangePlayerTurn();

// global variables
// try not to access grid directly
Tile Grid[COLUMN][ROW];
Player Player_One, Player_Two;
Player *Current_Player;
Player *Winner;
Gamemode Current_Gamemode;
DifficultyMode gameDifficultyMode;
State Current_State;
Texture2D cross_circle_texture;

// current grid design, row = 3, column = 3
// 0,0 | 0,1 | 0,2
// 1,0 | 1,1 | 1,2
// 2,0 | 2,1 | 2,2
// index of each grid

// zihao testing
void HandleTilePlacement();
void showWinMenu();
int isBoardFull();
int miniMax(int depth, int max_depth, int is_max);
void miniMaxMakeBestMove();

int main(void)
{
    Init();

    // main game loop
    while (!WindowShouldClose())
    {
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
        }
    }

    CloseWindow();
    return 0;
}

void Init()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "tic tac toeeee");
    cross_circle_texture = LoadTexture("assets/tictactoe.png");
    SetExitKey(0); // prevent esc from closing the window
}

// render update loop
// functions related to rendering should reside here
void UpdateGameRender()
{
    if (Winner != NULL || isBoardFull())
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
    else
    {
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
        Current_State = PAUSE;
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
                miniMaxMakeBestMove();
                ChangePlayerTurn();
            }
            break;
        case AI_ML:
            break;
    }

    CheckWinCondition();
}

// Function to handle tile placement logic
void HandleTilePlacement()
{
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse_position = GetMousePosition();
        int tile_x = mouse_position.x / CELL_WIDTH;
        int tile_y = mouse_position.y / CELL_HEIGHT;

        if (SetTile(tile_x, tile_y, Current_Player->tile))
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
        Current_State = GAME;
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Settings"))
    {
        Current_State = SETTING;
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
        Current_State = MENU;
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
        Current_State = MENU;
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
        Current_State = GAME;
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        Current_State = GAME;
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 2.4, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        Current_State = MENU;

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
    else if (isBoardFull() && Winner == NULL)
    {
        TITLE = "Draw!";
    }

    const float HALF_SCREEN_WIDTH = SCREEN_WIDTH / 2;
    const float HALF_SCREEN_HEIGHT = SCREEN_HEIGHT / 2;

    DrawText(TITLE, HALF_SCREEN_WIDTH - MeasureText(TITLE, 60) / 2, HALF_SCREEN_HEIGHT / 2, 60, BLACK);

    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Restart"))
    {
        Current_State = GAME;
        StartGame();
    }
    if (GuiButton((Rectangle){HALF_SCREEN_WIDTH - BUTTON_WIDTH / 2, HALF_SCREEN_HEIGHT - BUTTON_HEIGHT / 2 + BUTTON_HEIGHT * 1.2, BUTTON_WIDTH, BUTTON_HEIGHT}, "Return to Main Menu"))
        Current_State = MENU;

    EndDrawing();
}

// main grid rendering function
void RenderGrid()
{
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            int x_coord = j * CELL_WIDTH;
            int y_coord = i * CELL_HEIGHT;
            int CELL_HALF_WIDTH = CELL_WIDTH / 2;

            DrawRectangleLines(x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT, BLACK);

            RenderTile(i, j, Grid[i][j]);
            const char *tile_index = TextFormat("[%d, %d]", i, j);
            DrawText(tile_index, x_coord + CELL_HALF_WIDTH - MeasureText(tile_index, 20) / 2, y_coord + CELL_HALF_WIDTH - 10, 20, BLACK);
        }
}

// function to render a specific tile
void RenderTile(int x, int y, Tile tile)
{
    Rectangle destination = {x * CELL_WIDTH, y * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT};
    Rectangle source;

    switch (Grid[x][y])
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
}

// setter for tile
// use this instead of accessing array directly
bool SetTile(int x, int y, Tile tile)
{
    if (IsTilePlaceable(x, y))
    {
        Grid[x][y] = tile;
        return true;
    }
    else
        return false;
}

// check if tile is empty and able to place
bool IsTilePlaceable(int x, int y)
{
    if (Grid[x][y] == EMPTY)
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

// Functions check if board is full return 1 if full else 0
int isBoardFull()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (Grid[i][j] == EMPTY)
            {
                return 0;
            }
        }
    }
    return 1;
}

// Minimax algorithm with alpha-beta pruning
int miniMax(int depth, int is_max, int max_depth)
{
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
    else if(isBoardFull() || depth == max_depth) // Check if board is full or depth is max, return 0
    {
        return 0;
    }

    // initalize a best value base on the current player (max or min)
    int best = is_max ? -1000 : 1000;

    // Loop through the board cells
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (Grid[i][j] == EMPTY)
            {
                // Temporarily set the cell with the current player's tile
                Grid[i][j] = is_max ? CROSS : CIRCLE;

                // Recursively calculate the minimax value
                int move_val = miniMax(depth + 1, !is_max, max_depth);

                // Update the best value based on the current player (max or min)
                best = is_max ? fmax(best, move_val) : fmin(best, move_val);

                // Undo the move (backtrack)
                Grid[i][j] = EMPTY;
            }
        }
    }

    return best;
}

void miniMaxMakeBestMove()
{   
    // Set initial difficult of miniMax mod to easy to look only 1 move ahead
    int difficulty = 1;

    // if difficulty is medium or hard update difficulty
    if(gameDifficultyMode == MEDIUM)
        difficulty = 3; // look 3 moves ahead
    else if(gameDifficultyMode == HARD)
        difficulty = 8; // look all moves ahead

    // initial best value to a very low value
    int best_val = -1000;

    // initialize 2d array row and column value
    int best_move_row = -1;
    int best_move_column = -1;

    // Loop through the board cells
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            // if cell is empty, attempt move and see if it is the best move
            if (Grid[i][j] == EMPTY)
            {
                Grid[i][j] = CROSS;
                int move_val = miniMax(0, 0, difficulty);
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
    Grid[best_move_row][best_move_column] = CROSS;
}