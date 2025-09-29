#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Ilibs -Ilibs/aether/vm-include -Ilibs/aether/ir-include"
LDFLAGS="-z execstack"
CLIENT_SRC="$(find client-src -name "*.c")"
SERVER_SRC="$(find server-src -name "*.c")"
INTP_SRC="$(find intp-src -name "*.c")"
AETHER_IR_SRC="$(find libs/aether/ir-src -name "*.c")"
AETHER_VM_SRC="$(find libs/aether/vm-src -name "*.c" -not -name "io.c")"

AETHER_COMPILER_SRC="$(find libs/aether/compiler-src -name "*.c")"
LEXGEN_SRC="$(find libs/lexgen/runtime-src -name "*.c")"

lexgen libs/aether/compiler-src/grammar.h libs/aether/compiler-src/grammar.lg
cc -o aetherc -Wall -Wextra -Ilibs/aether/ir-src -Ilibs -Ilibs/aether/ir-include \
  -z execstack "${@:1}" $AETHER_COMPILER_SRC $AETHER_IR_SRC $LEXGEN_SRC

AETHER_TEST_VM_SRC="$(find libs/aether/test-vm-src -name "*.c")"

cc -o aether-vm -Wall -Wextra -Ilibs/aether/ir-src -Ilibs -Ilibs/aether/vm-include \
  -Ilibs/aether/ir-include -z execstack "${@:1}" $AETHER_TEST_VM_SRC \
  $AETHER_VM_SRC $AETHER_IR_SRC

cc -o inc $CFLAGS $LDFLAGS "${@:1}" $CLIENT_SRC $INTP_SRC $AETHER_IR_SRC $AETHER_VM_SRC
cc -o ins $CFLAGS $LDFLAGS "${@:1}" $SERVER_SRC $INTP_SRC $AETHER_IR_SRC $AETHER_VM_SRC
