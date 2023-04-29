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


# Performance
Performance statistics are stored in "PerformanceStats.csv". They are eveluated using GCC and executed one at a time.