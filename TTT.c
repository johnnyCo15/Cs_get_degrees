// Single-player Tic-Tac-Toe with bot difficulties and win/loss counter

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3

typedef enum {
	DIFF_EASY = 1,
	DIFF_MEDIUM = 2,
	DIFF_HARD = 3
} Difficulty;

char board[SIZE][SIZE];

void initializeBoard() {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			board[i][j] = '-';
		}
	}
}

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

char checkWinner() {
	// rows
	for (int i = 0; i < SIZE; i++) {
		if (board[i][0] != '-' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
			return board[i][0];
		}
	}
	// cols
	for (int j = 0; j < SIZE; j++) {
		if (board[0][j] != '-' && board[0][j] == board[1][j] && board[1][j] == board[2][j]) {
			return board[0][j];
		}
	}
	// diags
	if (board[0][0] != '-' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
		return board[0][0];
	}
	if (board[0][2] != '-' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
		return board[0][2];
	}
	return '-';
}

int isBoardFull() {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == '-') return 0;
		}
	}
	return 1;
}

int isGameOver() {
	char w = checkWinner();
	if (w == 'X' || w == 'O') return 1;
	if (isBoardFull()) return 2; // draw
	return 0;
}

int isValidMove(int row, int col) {
	return row >= 0 && row < SIZE && col >= 0 && col < SIZE && board[row][col] == '-';
}

void placeMark(int row, int col, char mark) {
	if (isValidMove(row, col)) {
		board[row][col] = mark;
	} else {
		printf("Invalid move. Try again.\n");
	}
}

int tryFindWinningMove(char mark, int *outRow, int *outCol) {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == '-') {
				board[i][j] = mark;
				if (checkWinner() == mark) {
					*outRow = i; *outCol = j;
					board[i][j] = '-';
					return 1;
				}
				board[i][j] = '-';
			}
		}
	}
	return 0;
}

void botMoveEasy(char botMark) {
	int empties[SIZE * SIZE][2];
	int n = 0;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == '-') {
				empties[n][0] = i;
				empties[n][1] = j;
				n++;
			}
		}
	}
	if (n > 0) {
		int k = rand() % n;
		board[empties[k][0]][empties[k][1]] = botMark;
	}
}

void botMoveMedium(char botMark, char humanMark) {
	int r = -1, c = -1;
	// 1) win if possible
	if (tryFindWinningMove(botMark, &r, &c)) {
		board[r][c] = botMark;
		return;
	}
	// 2) block opponent's win
	if (tryFindWinningMove(humanMark, &r, &c)) {
		board[r][c] = botMark;
		return;
	}
	// 3) else random
	botMoveEasy(botMark);
}

int minimax(char currentMark, char botMark, char humanMark, int depth, int isMaximizing) {
	char winner = checkWinner();
	if (winner == botMark) return 10 - depth;
	if (winner == humanMark) return depth - 10;
	if (isBoardFull()) return 0;

	int bestScore = isMaximizing ? -1000 : 1000;
	char player = isMaximizing ? botMark : humanMark;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == '-') {
				board[i][j] = player;
				int score = minimax(player, botMark, humanMark, depth + 1, !isMaximizing);
				board[i][j] = '-';
				if (isMaximizing) {
					if (score > bestScore) bestScore = score;
				} else {
					if (score < bestScore) bestScore = score;
				}
			}
		}
	}
	return bestScore;
}

void botMoveHard(char botMark, char humanMark) {
	int bestScore = -1000;
	int bestR = -1, bestC = -1;
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == '-') {
				board[i][j] = botMark;
				int score = minimax(botMark, botMark, humanMark, 0, 0);
				board[i][j] = '-';
				if (score > bestScore) {
					bestScore = score;
					bestR = i; bestC = j;
				}
			}
		}
	}
	if (bestR != -1) {
		board[bestR][bestC] = botMark;
	} else {
		botMoveEasy(botMark);
	}
}

void botMove(Difficulty diff, char botMark, char humanMark) {
	if (diff == DIFF_EASY) botMoveEasy(botMark);
	else if (diff == DIFF_MEDIUM) botMoveMedium(botMark, humanMark);
	else botMoveHard(botMark, humanMark);
}

int readIntInRange(const char *prompt, int minVal, int maxVal) {
	int x;
	while (1) {
		printf("%s", prompt);
		if (scanf("%d", &x) == 1 && x >= minVal && x <= maxVal) {
			return x;
		}
		printf("Invalid input. Try again.\n");
		while (getchar() != '\n');
	}
}

char readYesNo(const char *prompt) {
	char ch;
	while (1) {
		printf("%s", prompt);
		if (scanf(" %c", &ch) == 1) {
			if (ch == 'y' || ch == 'Y') return 'y';
			if (ch == 'n' || ch == 'N') return 'n';
		}
		printf("Please enter y or n.\n");
	}
}

void readMove(int *outRow, int *outCol) {
	while (1) {
		printf("Enter move as 'row col' (0-2 0-2): ");
		int r, c;
		int count = scanf("%d %d", &r, &c);
		if (count == 2) {
			if (r >= 0 && r < SIZE && c >= 0 && c < SIZE) {
				if (isValidMove(r, c)) {
					*outRow = r;
					*outCol = c;
					return;
				} else {
					printf("Cell occupied. Try again.\n");
				}
			} else {
				printf("Out of range. Try again.\n");
			}
		} else {
			printf("Invalid input. Try again.\n");
		}
		while (getchar() != '\n');
	}
}

int main() {
	srand((unsigned int)time(NULL));

	int wins = 0, losses = 0, draws = 0;
	printf("Tic Tac Toe (You vs Bot)\n");
	while (1) {
		initializeBoard();

		printf("Select difficulty: 1) Easy  2) Medium  3) Hard\n");
		int d = readIntInRange("Enter 1-3: ", 1, 3);
		Difficulty diff = (Difficulty)d;

		char goFirst = readYesNo("Do you want to go first? (y/n): ");
		char human = (goFirst == 'y') ? 'X' : 'O';
		char bot = (human == 'X') ? 'O' : 'X';
		int humanTurn = (human == 'X');

		while (1) {
			printf("\n");
			printBoard();
			if (humanTurn) {
				int row, col;
				readMove(&row, &col);
				placeMark(row, col, human);
			} else {
				botMove(diff, bot, human);
				printf("Bot played.\n");
			}

			int state = isGameOver();
			if (state == 1) {
				printf("\n");
				printBoard();
				char w = checkWinner();
				if (w == human) {
					printf("You win!\n");
					wins++;
				} else {
					printf("Bot wins.\n");
					losses++;
				}
				break;
			} else if (state == 2) {
				printf("\n");
				printBoard();
				printf("It's a draw.\n");
				draws++;
				break;
			}

			humanTurn = !humanTurn;
		}

		printf("\nScore -> Wins: %d  Losses: %d  Draws: %d\n", wins, losses, draws);
		char again = readYesNo("Play again? (y/n): ");
		if (again == 'n') {
			printf("Thanks for playing!\n");
			break;
		}
	}

	return 0;
}
