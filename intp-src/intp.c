#define INTP_NO_EXTERN
#include "intp.h"
#include "shl_defs.h"

char *server_messages[INTPReturnCodesCount] = {
  [INTPReturnCodeSuccess] = "Success",
  [INTPReturnCodeCorruptedHeader] = "Corrupted header",
  [INTPReturnCodeWrongBytecodeSize] = "Wrong bytecode size",
};
