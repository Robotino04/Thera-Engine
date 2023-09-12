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
AB-pruning + transpositions + no checkmate in eval + move preordering: 5.17sa

## `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w QKqk - 0 1` (Depth 8)
AB-pruning + transpositions: 60+ s
AB-pruning + transpositions + no checkmate in eval: 60+ s
AB-pruning + transpositions + no checkmate in eval + move preordering: 8.46s

```pgn
[Event "My Tournament"]
[Site "?"]
[Date "2023.09.11"]
[Round "15"]
[White "Thera newest"]
[Black "Thera Base"]
[Result "*"]
[ECO "A00"]
[GameStartTime "2023-09-11T09:55:50.860 CEST"]
[Opening "Benko's Opening"]
[Termination "unterminated"]
[TimeControl "0.3/move"]

1. g3 {0.00/3 0.31s} Na6 {0.00/3 0.30s} 2. c3 {0.00/4 0.30s} Nc5 {+0.50/3 0.30s}
3. Bg2 {0.00/4 0.30s} e6 {0.00/3 0.30s} 4. c4 {0.00/4 0.30s} Qf6 {+1.00/3 0.30s}
5. f3 {-1.00/4 0.30s} Qd4 {+1.50/3 0.30s} 6. Qc2 {-1.50/4 0.30s}
Nf6 {+1.50/3 0.30s} 7. f4 {-1.50/4 0.30s} h6 {+1.50/3 0.30s}
8. Nf3 {-1.00/4 0.30s} Qd6 {+1.00/3 0.30s} 9. Ng1 {-1.00/4 0.30s}
Qd4 {+1.50/3 0.30s} 10. Nf3 {-1.00/4 0.30s} Qd6 {+1.00/3 0.30s}
11. Bh3 {-1.00/4 0.30s} Qc6 {+1.50/3 0.30s} 12. O-O {-1.00/4 0.30s}
Na6 {+1.00/3 0.30s} 13. Qc3 {-1.00/4 0.30s} Bc5+ {+3.00/3 0.30s}
14. e3 {+2.00/5 0.30s} Bb4 {+3.00/3 0.30s} 15. Qc2 {-1.00/4 0.30s}
Bc5 {+1.50/3 0.30s} 16. a3 {-1.50/4 0.30s} b6 {+1.50/3 0.30s}
17. Nh4 {-1.00/4 0.30s} Ne4 {+1.00/3 0.30s} 18. Bg2 {0.00/4 0.30s}
g5 {0.00/3 0.30s} 19. Bxe4 {+6.00/5 0.30s} d5 {-1.00/3 0.30s}
20. Bxd5 {+4.00/5 0.30s} exd5 {-0.50/3 0.30s} 21. fxg5 {+3.00/5 0.30s}
hxg5 {+0.50/3 0.30s} 22. Nf3 {+3.00/5 0.30s} dxc4 {+0.50/3 0.30s}
23. Qc3 {+3.00/5 0.30s} f6 {+1.50/3 0.30s} 24. Nxg5 {-1.00/4 0.30s}
b5 {+2.00/3 0.30s} 25. Rxf6 {+5.00/5 0.30s} Qd5 {0.00/3 0.30s}
26. Re6+ {+5.50/5 0.30s} Bxe6 {-5.50/4 0.30s} 27. Qxh8+ {+1.00/6 0.30s}
Bg8 {-4.00/4 0.30s} 28. Qg7 {+4.00/5 0.30s} O-O-O {-0.50/3 0.30s}
29. Qh6 {+1.00/4 0.30s} Kb7 {-0.50/3 0.30s} 30. Nc3 {0.00/4 0.30s}
Qe5 {0.00/3 0.30s} 31. Nxb5 {+1.00/4 0.30s} Bf8 {0.00/3 0.30s}
32. Qg6 {+6.00/5 0.30s} Qxb5 {0.00/3 0.31s} 33. Qxg8 {0.00/4 0.30s}
Be7 {0.00/3 0.30s} 34. a4 {+3.00/5 0.30s} Bxg5 {0.00/3 0.30s}
35. Qxd8 {+4.00/6 0.30s} Qe5 {-3.50/3 0.30s} 36. Qg8 {+7.00/5 0.30s}
Qc5 {-4.00/3 0.30s} 37. Qe6 {+5.00/4 0.30s} Qd6 {-5.00/3 0.30s}
38. Qg8 {+7.00/5 0.30s} Qc5 {-4.00/3 0.30s} 39. Qe6 {+5.00/4 0.30s}
c3 {-5.00/3 0.30s} 40. bxc3 {+8.50/5 0.30s} Qb6 {-6.00/3 0.30s}
41. Qd5+ {+8.50/6 0.30s} c6 {-9.00/4 0.30s} 42. Qxg5 {+8.00/6 0.30s}
Nc7 {-8.00/3 0.30s} 43. Kf2 {+8.00/4 0.30s} Ka6 {-8.00/3 0.30s}
44. Qg4 {+9.00/4 0.30s} Kb7 {-8.00/3 0.30s} 45. Qb4 {+9.00/4 0.30s}
a5 {-9.00/3 0.30s} 46. Qd6 {+10.50/5 0.30s} Qb3 {-8.00/3 0.30s}
47. Qd3 {+8.00/4 0.30s} Qb6 {-8.00/3 0.30s} 48. Rb1 {+10.50/4 0.30s}
Nb5 {-12.00/4 0.30s} 49. Kf1 {+11.00/4 0.30s} Qc7 {-11.00/3 0.30s}
50. axb5 {+11.00/4 0.30s} Qf7+ {-11.00/3 0.30s} 51. Ke1 {+12.50/5 0.30s}
Kc7 {-12.00/3 0.30s} 52. Qe4 {+13.00/5 0.30s} cxb5 {-11.50/3 0.30s}
53. Rxb5 {+13.00/5 0.30s} Qg7 {-12.00/3 0.30s} 54. Rb7+ {+21.50/5 0.30s}
Kd6 {-M6/4 0.17s} 55. Rxg7 {+22.00/5 0.30s} Kc5 {-M6/4 0.030s}
56. Qd3 {+21.00/4 0.30s} Kc6 {-22.00/5 0.30s} 57. Qd8 {+22.50/5 0.30s}
a4 {-22.00/4 0.31s} 58. Qh4 {+22.50/5 0.30s} Kb6 {-22.50/4 0.30s}
59. Qb4+ {+22.00/4 0.30s} Kc6 {-M8/5 0.13s} 60. Rg6+ {+22.00/4 0.30s}
Kc7 {-M6/4 0.10s} 61. Rg8 {+22.00/4 0.30s} Kd7 {-22.50/4 0.31s}
62. c4 {+22.00/4 0.30s} Ke6 {-22.50/4 0.31s} 63. Rd8 {+22.00/4 0.30s}
a3 {-22.50/4 0.31s} 64. Qb6+ {+22.00/4 0.30s} Kf5 {-22.50/4 0.30s}
65. Qc5+ {+22.00/4 0.30s} Kg6 {-22.50/4 0.32s} 66. Qa5 {+22.00/4 0.31s}
a2 {-22.50/4 0.30s} 67. Rd7 {+22.00/4 0.30s} a1=Q {-22.50/4 0.31s}
68. Qxa1 {+22.00/6 0.30s} Kf5 {-22.50/4 0.31s} 69. h4 {+22.00/4 0.30s}
Kg4 {-22.50/4 0.30s} 70. Kf2 {+22.50/5 0.30s} Kh3 {-22.50/4 0.30s}
71. Bb2 {+22.50/5 0.30s} Kg4 {-22.00/5 0.30s} 72. d4 {+22.00/4 0.30s}
Kh5 {-22.50/4 0.31s} 73. Ke1 {+22.00/4 0.30s} Kg4 {-22.00/5 0.30s}
74. Kf2 {+22.50/5 0.30s} Kh5 {-22.50/4 0.32s} 75. Rg7 {+22.00/4 0.30s}
Kh6 {-M8/5 0.062s} 76. Qa7 {+22.50/5 0.30s} Kh5 {-M6/3 0.001s}
77. Qf7+ {+22.50/5 0.30s} Kh6 {-M4/2 0s} 78. Qc7 {+22.00/6 0.30s}
Kh5 {-M6/3 0.001s} 79. Re7 {+22.50/5 0.30s} Kg4 {-M8/5 0.20s}
80. c5 {+22.00/4 0.30s} Kh3 {-22.00/5 0.30s} 81. Re4 {+22.50/5 0.30s}
Kh2 {-M6/4 0.009s} 82. Qa5 {+22.50/5 0.30s} Kh3 {-M6/4 0.014s}
83. c6 {+30.00/6 0.30s} Kh2 {-M6/4 0.012s} 84. c7 {+30.50/5 0.30s}
Kh3 {-M6/4 0.019s} 85. Qb4 {+30.50/5 0.30s} Kh2 {-M6/4 0.015s}
86. Re5 {+30.50/5 0.30s} Kh3 {-M6/4 0.024s} 87. Qb8 {+30.50/5 0.30s}
Kg4 {-M6/4 0.022s} 88. h5 {+30.50/5 0.30s} Kh3 {-M6/4 0.011s}
89. Qb4 {+30.50/5 0.30s} Kh2 {-M6/4 0.022s} 90. Rb5 {+30.50/5 0.30s}
Kh1 {-M6/4 0.041s} 91. Ra5 {+30.50/5 0.30s} Kh2 {-M6/4 0.012s}
92. Ra1 {+30.50/5 0.30s} Kh3 {-M6/4 0.013s} 93. Ra7 {+30.50/5 0.30s}
Kg4 {-M8/5 0.19s} 94. Ra5 {+30.50/5 0.30s} Kh3 {-M6/4 0.016s}
95. Rf5 {+30.50/5 0.30s} Kh2 {-M6/4 0.026s} 96. h6 {+30.50/5 0.30s}
Kh3 {-M6/4 0.029s} 97. c8=Q {+38.50/5 0.30s} Kg4 {-M4/2 0s}
98. h7 {+38.50/5 0.30s} Kh3 {-M4/2 0s} 99. h8=Q+ {+38.00/4 0.30s} Kg4 {-M4/2 0s}
100. Rf3+ {+38.50/5 0.30s} Kg5 {-M4/2 0s} 101. Qa3 {+38.00/4 0.30s}
Kg6 {-M4/2 0s} 102. Qb7 {+38.00/4 0.30s} Kg5 {-M4/2 0s}
103. Qh1 {+38.00/4 0.30s} Kg6 {-M6/3 0.004s} 104. Qg2 {+38.00/4 0.30s}
Kh6 {-38.50/4 0.30s} 105. Rf6+ {+38.00/4 0.30s} Kg5 {-M6/3 0.005s}
106. Ra6 {+38.00/4 0.30s} Kh5 {-M6/3 0.006s} 107. Ba1 {+38.00/4 0.30s}
Kg4 {-M6/3 0.004s} 108. Qd7+ {+38.00/4 0.30s} Kh5 {-M4/2 0.001s}
109. Qdb7 {+38.00/4 0.30s} Kg4 {-M6/3 0.006s} 110. Ke1 {+38.00/4 0.30s}
Kh5 {-M6/3 0.008s} 111. Qbb3 {+38.00/4 0.30s} Kg5 {-M6/3 0.005s}
112. Qga2 {+38.00/4 0.30s} Kh5 {-M6/4 0.12s} 113. Qd1+ {+38.00/4 0.30s}
Kg5 {-M4/2 0s} 114. Ra8 {+38.00/4 0.30s} Kg6 {-M6/4 0.18s}
115. Qa5 {+38.00/4 0.30s} Kh7 {-M6/4 0.18s} 116. Rf8 {+38.00/4 0.30s}
Kg6 {-M6/4 0.17s} 117. Qab3 {+38.00/4 0.30s} Kg7 {-M6/4 0.14s}
118. Qaa3 {+38.00/4 0.30s} Kh6 {-M6/4 0.19s} 119. Qbd3 {+38.00/4 0.30s}
Kg7 {-M6/3 0.004s} 120. Kd2 {+38.00/4 0.30s} Kh6 {-M6/3 0.002s}
121. Rf7 {+38.00/4 0.30s} Kg5 {-M6/3 0.002s} 122. Qb5+ {+38.00/4 0.30s}
Kg6 {-M4/2 0.002s} 123. Qe7 {+38.00/4 0.30s} Kh6 {-M4/2 0s} *

```