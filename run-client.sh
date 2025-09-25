#!/usr/bin/sh

./build.sh "${@:2}" && ./aetherc temp.ac $1 && ./inc temp.ac
