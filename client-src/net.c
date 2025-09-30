#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "net.h"
#include "../intp-src/intp.h"
#include "shl_log.h"

Str get_bytecode_from_server(char *address, u32 port, char *route) {
  i32 client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    ERROR("Failed to open socket\n");
    exit(1);
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &server_address.sin_addr) <= 0) {
    printf("Invalid server address\n");
    exit(1);
  }

  if (connect(client_socket,
              (struct sockaddr*) &server_address,
              sizeof(server_address)) < 0) {
    ERROR("Failed to connect to server\n");
    exit(1);
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
    exit(1);
  }

  if (!str_eq(STR(header, PREFIX_SIZE), STR_LIT(PREFIX))) {
    ERROR("Corrupted bytecode\n");
    exit(1);
  }

  u16 return_code = *(u16 *) (header + RETURN_CODE_OFFSET);
  if (return_code != INTPReturnCodeSuccess) {
    ERROR("%s\n", server_messages[return_code]);
    exit(1);
  }

  u32 bytecode_size = *(u32 *) (header + PAYLOAD_SIZE_OFFSET);
  char *bytecode = malloc(bytecode_size);

  len = read(client_socket, bytecode, bytecode_size);
  if (len < bytecode_size) {
    ERROR("Corrupted bytecode\n");
    exit(1);
  }

  close(client_socket);

  return (Str) { bytecode, bytecode_size };
}
