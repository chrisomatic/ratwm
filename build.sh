#!/bin/sh
rm -rf bin
mkdir bin

gcc main.c \
    -lX11 \
    -lGL \
    -lm \
    -o bin/ratwm
