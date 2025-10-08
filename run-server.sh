#!/usr/bin/sh

aetherc test.ac $1 && aether server-src/main.ae ${@:2}
