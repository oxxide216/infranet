#!/usr/bin/sh

aetherc test.ac $1 && aether server-aether-src/main.ae ${@:2}
