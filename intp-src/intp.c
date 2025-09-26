#define INTP_NO_EXTERN
#include "intp.h"
#include "../client-src/shl_log.h"

char *server_messages[INTPReturnCodesCount] = {
  [INTPReturnCodeSuccess] = "Success",
  [INTPReturnCodeCorruptedHeader] = "Corrupted header",
  [INTPReturnCodeWrongRouteSize] = "Wrong route size",
  [INTPReturnCodeRouteWasNotFound] = "Route was not found",
};

void *create_message(u32 return_code, u32 payload_size,
                     void *payload, u32 *message_size) {
  *message_size = HEADER_SIZE + payload_size;
  void *message = malloc(*message_size);

  memcpy(message, PREFIX, PREFIX_SIZE);
  *(u32 *) (message + RETURN_CODE_OFFSET) = return_code;
  *(u32 *) (message + PAYLOAD_SIZE_OFFSET) = payload_size;
  memcpy(message + PAYLOAD_OFFSET, payload, payload_size);

  return message;
}
