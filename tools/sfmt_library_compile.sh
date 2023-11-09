#!/bin/bash
gcc -c -DSFMT_MEXP=19937 -fpic -Werror  SFMT.c
gcc -shared -o libSFMT.so SFMT.o
