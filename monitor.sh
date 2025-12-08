#!/usr/bin/env bash

tail -F /tmp/match.log | awk '
  /^(Score of )|(Rank Name )/ {inblock=1; block=$0 ORS; next}
  inblock {
    if (/^SPRT: llr /) {
      block = block $0 ORS      # append the last line once
      system("clear")
      printf "%s", block
      inblock=0                  # reset for next block
      block=""
    } else {
      block = block $0 ORS       # append normal lines
    }
  }
'
