#include <netinet/in.h>
#include <unistd.h>

#include "../intp-src/intp.h"
#include "aether-vm/vm.h"
#include "aether-ir/deserializer.h"
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#define PORT 8080

int main(i32 argc, char **argv) {
  i32 server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    ERROR("Failed to open socket\n");
    return 1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr*) &address,
           sizeof(address)) < 0) {
    ERROR("Failed to bind socket\n");
    return 1;
  }

  if (listen(server_socket, 3) < 0) {
    ERROR("Failed to listen new connections\n");
    return 1;
  }

  while (true) {
    u32 address_size;
    i32 client_socket = accept(server_socket,
                               (struct sockaddr*) &address,
                               &address_size);
    if (client_socket < 0) {
        ERROR("Failed to accept new connection\n");
        return 1;
    }

    u32 return_code = INTPReturnCodeSuccess;
    char header[HEADER_SIZE + 1] = {0};

    u32 len = read(client_socket, header, HEADER_SIZE);
    if (len < HEADER_SIZE ||
        !str_eq(STR(header, PREFIX_LEN), STR_LIT(PREFIX))) {
      return_code = INTPReturnCodeCorruptedHeader;
    } else {
      u32 bytecode_len = ((u32 *) header)[sizeof(PREFIX) / sizeof(u32)];
      char *bytecode = malloc(bytecode_len);
      len = read(client_socket, bytecode, bytecode_len);

      if (len < bytecode_len) {
        return_code = INTPReturnCodeWrongBytecodeSize;
      } else {
        RcArena rc_arena = {0};
        Ir ir = deserialize((u8 *) bytecode, bytecode_len, &rc_arena);
        Intrinsics intrinsics = {0};
        execute(&ir, argc, argv, &rc_arena, &intrinsics);
      }

      free(bytecode);
    }

    send(client_socket, &return_code, 2, 0);

    close(client_socket);
  }

  close(server_socket);

  return 0;
}
