#ifndef INTP_H
#define INTP_H

#include "shl_defs.h"

#define HEADER_SIZE 6
#define PREFIX      "INTP"
#define PREFIX_LEN  (sizeof(PREFIX) - 1)

typedef enum {
  INTPReturnCodeSuccess = 0,
  INTPReturnCodeCorruptedHeader,
  INTPReturnCodeWrongBytecodeSize,
  INTPReturnCodesCount,
} INTPReturnCode;

#ifndef INTP_NO_EXTERN
extern char *server_messages[INTPReturnCodesCount];
#endif

#endif // INTP_H
