#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../intp-src/intp.h"
#include "aether-vm/vm.h"
#include "aether-ir/deserializer.h"
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#define DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_PORT           8080

int main(i32 argc, char **argv) {
  char *route = "";

  if (argc > 1)
    route = argv[1];

  i32 client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    ERROR("Failed to open socket\n");
    return 1;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(DEFAULT_PORT);

  if (inet_pton(AF_INET, DEFAULT_SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
    printf("Invalid server address\n");
    return 1;
  }

  if (connect(client_socket,
              (struct sockaddr*) &server_address,
              sizeof(server_address)) < 0) {
    ERROR("Failed to connect to server\n");
    return 1;
  }

  u32 message_size;
  void *message = create_message(0, strlen(route),
                                 route, &message_size);

  send(client_socket, message, message_size, 0);

  free(message);

  char header[HEADER_SIZE] = {0};
  u32 len = read(client_socket, header, HEADER_SIZE);
  if (len < HEADER_SIZE) {
    ERROR("Corrupted server respone\n");
    return 1;
  }

  if (!str_eq(STR(header, PREFIX_SIZE), STR_LIT(PREFIX))) {
    ERROR("Corrupted bytecode\n");
    return 1;
  }

  u16 return_code = *(u16 *) (header + RETURN_CODE_OFFSET);
  if (return_code != INTPReturnCodeSuccess) {
    ERROR("%s\n", server_messages[return_code]);
    return 1;
  }

  u32 bytecode_size = *(u32 *) (header + PAYLOAD_SIZE_OFFSET);
  char *bytecode = malloc(bytecode_size);

  len = read(client_socket, bytecode, bytecode_size);
  if (len < bytecode_size) {
    ERROR("Corrupted bytecode\n");
    return 1;
  }

  RcArena rc_arena = {0};
  Ir ir = deserialize((u8 *) bytecode, bytecode_size, &rc_arena);
  Intrinsics intrinsics = {0};
  execute(&ir, argc, argv, &rc_arena, &intrinsics);

  free(bytecode);

  close(client_socket);

  return 0;
}
