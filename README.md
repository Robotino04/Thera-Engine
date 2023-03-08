# Thera Engine
Thera is a homemade chess engine written in C++.


# Debugging
For debugging logic errors in move generation, the following command can be used to compare theras output with e.g. stockfish.

``` bash
diff <(sort /tmp/thera.txt) <(sort /tmp/solution.txt)
```

To check bitboard patterns, this python code could help.

``` python
print("\n".join([ bin(0xfefefefefefefefe)[2:].rjust(64, "0")[i:i+8][::-1] for i in range(0, 64, 8)][::-1]))
```