# Thera Engine
Thera is a homemade chess engine written in C++.


# Debugging
For debugging logic errors in move generation, the following command can be used to compare theras output with e.g. stockfish.

``` bash
diff <(sort /tmp/thera.txt) <(sort /tmp/solution.txt)
```

# TODO
[] replace headers like "stdint.h" with "cstdint"