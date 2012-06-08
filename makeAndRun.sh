#!/bin/sh
gcc -lreadline -lefence -ggdb -O4 -Wall -std=c99 -D_GNU_SOURCE -o toysh toysh.c parser.c dictionary.c && ./toysh
