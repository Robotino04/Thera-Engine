# Thera Engine
Thera is a homemade chess engine written in C++.


# Debugging
For debugging logic errors in move generation, the following command can be used to compare theras output with e.g. stockfish.

``` bash
diff <(sort /tmp/thera.txt) <(sort /tmp/solution.txt)
```

There is also the cli command `analyze` that compares the generated moves between Thera and Stockfish.

To check bitboard patterns, this python code could help.

``` python
def printBB(bb): print("\n".join([ bin(bb)[2:].rjust(64, "0")[i:i+8][::-1] for i in range(0, 64, 8)]))
```

To test the engine more effectively and find potential bugs, I found [this](http://bernd.bplaced.net/fengenerator/fengenerator.html) tool to generate absurd chess positions.


# Performance
Performance statistics are stored in "PerformanceStats.csv". They are eveluated using GCC and executed one at a time.

## `8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1` (Depth 18)
AB-pruning + transpositions: 6.67s
AB-pruning + transpositions + no checkmate in eval: 4.25s
