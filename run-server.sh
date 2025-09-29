#!/usr/bin/sh

./build.sh && ./aetherc test.ac $1 && ./ins ${@:2}
