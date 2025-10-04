#!/usr/bin/sh

CLIENT_CFLAGS="-Wall -Wextra -Ilibs -Ilibs/aether/vm-include -Ilibs/aether/ir-include"
SERVER_CFLAGS="-Wall -Wextra -Ilibs -Ilibs/aether/vm-include -Ilibs/aether/ir-include"
LDFLAGS="-z execstack"
CLIENT_SRC="$(find client-src -name "*.c")"
SERVER_SRC="$(find server-src -name "*.c")"
INTP_SRC="$(find intp-src -name "*.c")"
AETHER_IR_SRC="$(find libs/aether/ir-src -name "*.c")"
AETHER_VM_SRC="$(find libs/aether/vm-src -name "*.c" -not -name "io.c")"

if [[ -z $NDEBUG ]]; then
  CLIENT_CFLAGS="$CLIENT_CFLAGS -DNDEBUG"
  SERVER_CFLAGS="$SERVER_CFLAGS -DNDEBUG"

  AETHER_COMPILER_SRC="$(find libs/aether/compiler-src -name "*.c")"
  LEXGEN_SRC="$(find libs/lexgen/runtime-src -name "*.c")"

  lexgen libs/aether/compiler-src/grammar.h libs/aether/compiler-src/grammar.lg
  cc -o aetherc -Wall -Wextra -Ilibs/aether/ir-src -Ilibs -Ilibs/aether/ir-include \
    -z execstack "${@:1}" $AETHER_COMPILER_SRC $AETHER_IR_SRC $LEXGEN_SRC

  AETHER_TEST_VM_SRC="$(find libs/aether/test-vm-src -name "*.c")"

  cc -o aether-vm -Wall -Wextra -Ilibs/aether/ir-src -Ilibs -Ilibs/aether/vm-include \
    -Ilibs/aether/ir-include -z execstack "${@:1}" $AETHER_TEST_VM_SRC \
    $AETHER_VM_SRC $AETHER_IR_SRC
fi

if [[ -z $NOIUI ]]; then
  CLIENT_CFLAGS="$CLIENT_CFLAGS -DIUI -lGL -lGLEW -lX11 -lm \
                 -Ialibs/iui/include -Ilibs/winx/include \
                 -Ilibs/glass/include"
  CLIENT_SRC="$CLIENT_SRC $(find alibs/iui/src -name "*.c") \
              $(find libs/winx/src -name "*.c") \
              $(find libs/glass/src -name "*.c")"

  xxd -i alibs/iui/fonts/JetBrainsMono-Regular.ttf > alibs/iui/fonts/JetBrainsMono-Regular.h
fi

cc -o inc $CLIENT_CFLAGS $LDFLAGS "${@:1}" $CLIENT_SRC $INTP_SRC $AETHER_IR_SRC $AETHER_VM_SRC
cc -o ins $SERVER_CFLAGS $LDFLAGS "${@:1}" $SERVER_SRC $INTP_SRC $AETHER_IR_SRC $AETHER_VM_SRC
