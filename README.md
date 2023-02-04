# ChessBot
A chess bot.


# Debugging
For debugging logic errors in move generation, the following command can be used to compare this codes output with e.g. stockfish.

``` bash
comm -2 -3 <(sort /tmp/chessBot.txt) <(sort /tmp/solution.txt)
```