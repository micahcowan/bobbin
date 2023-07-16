#!/usr/bin/env bash
FILE=$1
ca65 -l $FILE.lst $FILE.ca65
ld65 $FILE.o -o $FILE.bin -m $FILE.map -C example.cfg

