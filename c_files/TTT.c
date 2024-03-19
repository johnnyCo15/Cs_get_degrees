// code for tictactoe game with two human players

#include <stdio.h>
#include <stdlib.h>

#define SIZE 3

char board[SIZE][SIZE];

// Function to initialize the board
void initializeBoard() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = '-';
        }
    }
}

// Function to print the board
void printBoard() {
    printf("  0 1 2\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%d ", i);
        for (int j = 0; j < SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

// Function to check if the game is over
int isGameOver() {
    // Check rows
    for (int i = 0; i < SIZE; i++) {
        if (board[i][0] != '-' && board[i][0] == board[i][1] && board[i][0] == board[i][2]) {
            return 1;
        }
    }

    // Check columns
    for (int j = 0; j < SIZE; j++) {
        if (board[0][j] != '-' && board[0][j] == board[1][j] && board[0][j] == board[2][j]) {
            return 1;
        }
    }

    // Check diagonals
    if (board[0][0] != '-' && board[0][0] == board[1][1] && board[0][0] == board[2][2]) {
        return 1;
    }
    if (board[0][2] != '-' && board[0][2] == board[1][1] && board[0][2] == board[2][0]) {
        return 1;
    }

    // Check if board is full
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == '-') {
                return 0;
            }
        }
    }

    // If no winner and board is full, it's a draw
    return 2;
}

// Function to place a mark on the board
void placeMark(int row, int col, char mark) {
    if (row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == '-') {
        board[row][col] = mark;
    } else {
        printf("Invalid move. Try again.\n");
    }
}

int main() {
    initializeBoard();

    int currentPlayer = 0;
    char marks[2] = {'X', 'O'};
    int row, col, result;

    printf("Tic Tac Toe Game\n");
    printf("Player 1: X\nPlayer 2: O\n");

    while (1) {
        printf("\n");
        printBoard();

        printf("Player %d's turn. Enter row and column (0-2) separated by space: ", currentPlayer + 1);
        scanf("%d %d", &row, &col);

        placeMark(row, col, marks[currentPlayer]);

        result = isGameOver();
        if (result == 1) {
            printf("\n");
            printBoard();
            printf("Player %d wins!\n", currentPlayer + 1);
            break;
        } else if (result == 2) {
            printf("\n");
            printBoard();
            printf("It's a draw!\n");
            break;
        }

        currentPlayer = (currentPlayer + 1) % 2;
    }

    return 0;
}
