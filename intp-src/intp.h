#ifndef INTP_H
#define INTP_H

#include "shl_defs.h"

#define INTP_VERSION 1

#define HEADER_SIZE 12
#define PREFIX      "INTP"
#define PREFIX_SIZE (sizeof(PREFIX) - 1)

#define VERSION_OFFSET      PREFIX_SIZE
#define RETURN_CODE_OFFSET  (VERSION_OFFSET + sizeof(u16))
#define PAYLOAD_SIZE_OFFSET (RETURN_CODE_OFFSET + sizeof(u16))
#define PAYLOAD_OFFSET      HEADER_SIZE

typedef enum {
  INTPReturnCodeSuccess = 0,
  INTPReturnCodeCorruptedHeader,
  INTPReturnCodeWrongRouteSize,
  INTPReturnCodeRouteWasNotFound,
  INTPReturnCodesCount,
} INTPReturnCode;

#ifndef INTP_NO_EXTERN
extern char *server_messages[INTPReturnCodesCount];
#endif

void *create_message(u16 return_code, u32 payload_size,
                     void *payload, u32 *message_size);

#endif // INTP_H
