#!/usr/bin/sh

./build.sh && ./aetherc server.ac server-aether-src/main.ae && \
  ./aether-vm server.ac ${@:1}
