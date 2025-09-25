#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../intp-src/intp.h"
#include "io.h"
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#define DEFAULT_SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_PORT           8080

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Input file was not provided\n");
    return 1;
  }

  Str bytecode = read_file(argv[1]);
  if (bytecode.len == (u32) -1) {
    ERROR("Could not open %s\n", argv[1]);
    return 1;
  }

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

  u32 message_len = HEADER_SIZE + bytecode.len;
  char *message = malloc(message_len);
  memcpy(message, PREFIX, sizeof(PREFIX));
  ((u32 *) message)[sizeof(PREFIX) / sizeof(u32)] = bytecode.len;
  memcpy(message + HEADER_SIZE, bytecode.ptr, bytecode.len);

  send(client_socket, message, message_len, 0);

  free(message);

  u32 return_code;
  u32 len = read(client_socket, &return_code, 2);
  if (len < 2 || return_code >= INTPReturnCodesCount) {
    ERROR("Corrupted server return code\n");
  } else {
    if (return_code == 0)
      INFO("%s\n", server_messages[return_code]);
    else
      ERROR("%s\n", server_messages[return_code]);
  }

  close(client_socket);

  return 0;
}
