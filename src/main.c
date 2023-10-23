#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

typedef enum Gamemode {
    LOCAL,
    AI_MINIMAX,
    AI_ML
} Gamemode;

typedef enum Player {
    PLAYER_ONE,
    PLAYER_TWO,
    PLAYER_AI,
    PLAYER_NONE
} Player;

typedef enum Tile {
    EMPTY,
    CROSS,
    CIRCLE
} Tile;

typedef struct Cell {
    int x;
    int y;
    Tile value;
} Cell;

void Init();    
void UpdateRender();
void UpdateGame();
void RenderGrid();
void RenderTile(int x, int y, Tile tile);
void RenderTitle();

bool SetTile(int x, int y, Tile tile);
Tile GetTile(int x, int y);
bool IsTilePlaceable(int x, int y);
void PopulateGrid(Tile tile);
Player CheckWinCondition();

#define COLUMN 3
#define ROW 3
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
const int CELL_WIDTH = SCREEN_WIDTH / COLUMN;
const int CELL_HEIGHT = SCREEN_HEIGHT / ROW;

Tile Grid[COLUMN][ROW];
Player Current_Player;
Gamemode Current_Gamemode;

// current grid design
// 0,0 | 0,1 | 0,2
// 1,0 | 1,1 | 1,2
// 2,0 | 2,1 | 2,2
// index of each grid

int main(void) {
    Init();

    // main game loop
    while (!WindowShouldClose()) {
        UpdateGame();
        UpdateRender();
    }
    
    CloseWindow();
    return 0;
}

void Init()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "tic tac toeeee");
    PopulateGrid(EMPTY);
    Current_Player = PLAYER_ONE;
    Current_Gamemode = LOCAL;
}

// render update loop
// functions related to rendering should reside here
void UpdateRender()
{
    BeginDrawing();
    // clear screen and set white
    ClearBackground(RAYWHITE);
    RenderGrid();
    RenderTitle();
    EndDrawing();
}

// game logic update loop
// functions related to rendering should reside here
void UpdateGame()
{
    switch(Current_Gamemode)
    {
        case LOCAL:
            // receive user input and place tile
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse_position = GetMousePosition();
                int tile_x = mouse_position.x / CELL_WIDTH;
                int tile_y = mouse_position.y / CELL_HEIGHT;

                if (Current_Player == PLAYER_ONE)
                {
                    if (SetTile(tile_x, tile_y, CIRCLE))
                        Current_Player = PLAYER_TWO;
                }
                else if (Current_Player == PLAYER_TWO)
                {
                    if (SetTile(tile_x, tile_y, CROSS))
                        Current_Player = PLAYER_ONE;
                }
            }
            break;
        case AI_MINIMAX:
            break;
        case AI_ML:
            break;
    }

    CheckWinCondition();
}

// main grid rendering function
void RenderGrid()
{
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
        {
            int x_coord = i * CELL_WIDTH;
            int y_coord = j * CELL_HEIGHT;
            int CELL_HALF_WIDTH = CELL_WIDTH / 2;

            DrawRectangleLines(x_coord, y_coord, CELL_WIDTH, CELL_HEIGHT, BLACK);

            RenderTile(i, j, Grid[i][j]);
            const char* tile_index = TextFormat("[%d, %d]", i, j);
            DrawText(tile_index, x_coord + CELL_HALF_WIDTH - MeasureText(tile_index, 20) / 2, y_coord + CELL_HALF_WIDTH - 10, 20, BLACK);
        }
}

// function to render a specific tile
void RenderTile(int x, int y, Tile tile)
{
    int x_coord = x * CELL_WIDTH;
    int y_coord = y * CELL_HEIGHT;
    int CELL_HALF_WIDTH = CELL_WIDTH / 2;

    switch (Grid[x][y])
    {
                case CROSS:
                    DrawCircle(x_coord + CELL_HALF_WIDTH, y_coord + CELL_HALF_WIDTH, CELL_HALF_WIDTH / 2, GRAY);
                    break;
                case CIRCLE:
                    DrawCircle(x_coord + CELL_HALF_WIDTH, y_coord + CELL_HALF_WIDTH, CELL_HALF_WIDTH / 2, RED);
                    break;
                case EMPTY:
                    break;
    }
}

// render function for the title text
void RenderTitle()
{
    if (Current_Player == PLAYER_ONE)
        DrawText("Player 1's turn", 10, 10, 20, BLACK);
    else
        DrawText("Player 2's turn", 10, 10, 20, BLACK);
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

// getter for tile
Tile GetTile(int x, int y)
{
    return Grid[x][y];
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
Player CheckWinCondition()
{
    // todo !
    return PLAYER_NONE;
}