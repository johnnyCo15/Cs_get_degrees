# Cs_get_degrees
For those pesky projects that you want written in C just because you can.

## Projects

### TTT (Tic Tac Toe)
- Single-player Tic Tac Toe where you play against a bot.
- Three difficulties: Easy (random), Medium (win/block/random), Hard (minimax optimal).
- Tracks cumulative wins, losses, and draws across rounds.

#### Build & Run
From the repo root:

```bash
gcc -std=c11 -Wall -Wextra -O2 TTT.c -o TTT
./TTT
```

### datePicker
- Small C program(s) experimenting with date selection logic.
- Files: `datePicker.c`, `datePicker` (notes/config or sample data).

#### Build (if desired)

```bash
gcc -std=c11 -Wall -Wextra -O2 datePicker.c -o datePicker_app
./datePicker_app
```

## Notes
- Tested on macOS with Clang via `gcc` alias.
- No external dependencies.
