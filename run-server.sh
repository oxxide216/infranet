#!/usr/bin/sh

./build.sh && ./aetherc temp.ac $1 && ./ins ${@:2}
