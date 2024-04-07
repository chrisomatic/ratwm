#!/bin/sh

set -e

./build.sh

XEPHYR=$(whereis -b Xephyr | cut -f2 -d' ')
xinit ./xinitrc -- \
    "$XEPHYR" \
        :100 \
        -ac \
        -screen 1024x768 \
        -host-cursor
