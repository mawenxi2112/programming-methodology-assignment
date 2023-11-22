# CSC1103 Programming methodology

## Program sypnopsis

This C program is a Tic Tac Toe game developed using the [Raylib Library](https://github.com/raysan5/raylib), featuring a Graphical User Interface (GUI) and various game modes. The game supports multiplayer mode, player vs AI with a minimax algorithm at three difficulty levels (easy, medium, hard), and player vs Machine Learning using Naive Bayes classification. Additionally, the Machine Learning mode provides a confusion matrix for evaluating the testing results.

Project demo video can be found in this youtube link:  [https://youtu.be/Po9qWrZv5X4](https://youtu.be/Po9qWrZv5X4)

## Features

* **Graphical User Interface (GUI)**: The game utilizes raylib to create a interactive user interface

* **Multiplayer Mode**: Two players can play against each other on the same computer, taking turns to make their moves.

* **Player vs AI (Minimax algorithm)**: Players can challenge an AI opponent that uses the minimax algorithm for decision-making. The AI comes with three difficulty levels: easy, medium, and hard. Alpha-beta pruning was implemented to reduce total searchable branch improving performance.

* **Player vs Machine Learning (Naive Bayes)**: Players can challenge an ML opponent that uses machine learning model trained with the Naive Bayes classification algorithm. The testing results include a confusion matrix, providing insights into the model's performance.

## Project folders

`\src` contains the source code of the project.

`\resource` contains the various files (images / ML data) for the project.

`\bin` contains the compiled game and resource files for both Windows and MAC OSX.

`\raylib-master` contains the RayLib library (v4.5.0) used for compiling the game. (installation and set up of RayLib is needed)

## How to play

Download the game repository. You can either download the zip file, or clone the repository.

The executable is located in `\bin` as either `.\bin\tic_tac_toe_win.exe` for windows or `\bin\tic_tac_toe_mac` for Mac osx. Ensure `\resources` resides in the `\bin` folder before launching game.

The current executables in the `\bin` folders can be launch and played.

### Instructions to compile game

If current executables in the `\bin` folders are unable to run, the following instructions will provide a guide on how to compile the game for both Windows and Mac osx.

Delete both `\bin\tic_tac_toe_win.exe` or `\bin\tic_tac_toe_mac` depending on your system operating system before proceeding. Leave the resource folder untouched.

#### Download Raylib

Firstly ensure that Raylib is installed on your current computer

Installation guide for [Windows](https://github.com/raysan5/raylib/wiki/Working-on-Windows) and [MAC OSX](https://github.com/raysan5/raylib/wiki/Working-on-macOS)

Next make sure your present working directory is at the root folder of the project.

`ls -l` (mac) / `dir` (windows) should show makefile as one of of the files in the current directory.

#### Windows users

Ensure you are at the project root directory before running the following command.

```text
mingw32-make RAYLIB_PATH=raylib-master PROJECT_NAME=bin/tic_tac_toe_win.exe OBJS=src/tic_tac_toe.c
```

If `mingw32-make` is not recognized as an internal or external command,
operable program or batch file. User the full path instead.

```text
where mingw32-make.exe
```

#### Mac OSX users

Ensure you are at the project root directory before running the following command.

```text
make RAYLIB_PATH="$(find . -type d -name "raylib-master")" PROJECT_NAME=bin/tic_tac_toe_mac OBJS=src/tic_tac_toe.c
```

The generated executables are located at `\bin` as either `.\bin\tic_tac_toe_win.exe` for windows or `\bin\tic_tac_toe_mac` for Mac osx.

## Additional notes

Raylib installation is not required to launch the game but its required to compile the game. The `\bin` folder can be distributed as a standalone version of the game.

Raylib (v4.5.0) is included in the project folder in order to simplify steps to build the executables.

For troubleshooting of Raylib visit [Raylib Windows Guide](https://github.com/raysan5/raylib/wiki/Working-on-Windows) or [Raylib MAC OSX Guide](https://github.com/raysan5/raylib/wiki/Working-on-macOS).
