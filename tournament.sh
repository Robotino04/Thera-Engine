#!/usr/bin/env bash
cutechess-cli \
    -engine "conf=Stockfish 1320" \
    -engine "conf=Stockfish 1500" \
    -engine "name=Thera Latest" cmd=./target/release/thera \
    -engine "name=Thera Legacy" cmd=./build/UCI/TheraUCI \
    -openings file="book-ply6-unifen_Q.txt.dont_lsp" format=epd order=random plies=6 policy=default \
    -games 350 -concurrency 32 \
    -recover \
    -each tc=1 timemargin=200 proto=uci \
    -debug \
    -ratinginterval 10 | tee /tmp/match.log
