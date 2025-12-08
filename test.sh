#!/usr/bin/env bash
cutechess-cli \
    -engine "name=Thera Latest" cmd=./target/release/thera \
    -engine "name=Thera Reference" cmd=$1 \
    -games 999999 -concurrency 32 \
    -recover \
    -repeat 2 \
    -each tc=1 timemargin=200 proto=uci \
    -ratinginterval 10 \
    -openings file="book-ply6-unifen_Q.txt.dont_lsp" format=epd order=random plies=6 policy=default \
    -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 | tee /tmp/match.log
