#!/usr/bin/sh

if [[ $1 == "nonet" ]]; then
  ./build.sh -DNONET ${@:3} && \
    ./aetherc test.ac $2 && \
    ./inc test.ac
else
  ./build.sh ${@:2} && ./inc $1
fi
