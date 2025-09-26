#include <netinet/in.h>
#include <unistd.h>

#include "../intp-src/intp.h"
#include "aether-vm/vm.h"
#include "aether-ir/deserializer.h"
#include "io.h"
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#define PORT 8080

typedef struct {
  Str route;
  Str bytecode;
} RouteData;

typedef Da(RouteData) RoutesData;

static RoutesData parse_routes_data(i32 argc, char **argv) {
  RoutesData routes_data = {0};

  for (u32 i = 1; i < (u32) argc; ++i) {
    RouteData route_data;

    u32 arg_len = strlen(argv[i]);
    u32 j = arg_len;
    while (j > 0 && argv[i][j - 1] != ':')
      --j;

    if (j == 0) {
      ERROR("Wrong route forat\n");
      INFO("Expected format: route:file\n");
      exit(1);
    }

    route_data.route = STR(argv[i], j - 1);
    char *bytecode_file_path = argv[i] + j;
    route_data.bytecode = read_file(bytecode_file_path);
    if (route_data.bytecode.len == (u32) -1) {
      ERROR("Could not open %s\n", bytecode_file_path);
      exit(1);
    }

    INFO("Route initialized: "STR_FMT"\n", STR_ARG(route_data.route));

    DA_APPEND(routes_data, route_data);
  }

  return routes_data;
}

static u32 get_route_bytecode(i32 client_socket, Str *bytecode, RoutesData *routes_data) {
  bytecode->len = (u32) -1;
  char header[HEADER_SIZE] = {0};

  u32 len = read(client_socket, header, HEADER_SIZE);
  if (len < HEADER_SIZE ||
      !str_eq(STR(header, PREFIX_SIZE), STR_LIT(PREFIX)))
    return INTPReturnCodeCorruptedHeader;

  Str route;
  route.len = *(u32 *) (header + PAYLOAD_SIZE_OFFSET);
  if (route.len == 0) {
    for (u32 i = 0; i < routes_data->len; ++i) {
      if (routes_data->items[i].route.len == 0) {
        *bytecode = routes_data->items[i].bytecode;
        break;
      }
    }

    if (bytecode->len != (u32) -1)
      return INTPReturnCodeSuccess;
  }

  route.ptr = malloc(route.len);
  len = read(client_socket, route.ptr, route.len);
  if (len < route.len)
    return INTPReturnCodeWrongRouteSize;

  for (u32 i = 0; i < routes_data->len; ++i) {
    if (str_eq(routes_data->items[i].route, route)) {
      *bytecode = routes_data->items[i].bytecode;
      break;
    }
  }

  if (bytecode->len == (u32) -1)
    return INTPReturnCodeRouteWasNotFound;

  return INTPReturnCodeSuccess;
}

int main(i32 argc, char **argv) {
  if (argc < 2) {
    ERROR("Routes were not provided\n");
    return 1;
  }

  RoutesData routes_data = parse_routes_data(argc, argv);

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
      continue;
    }

    Str bytecode;
    u16 return_code = get_route_bytecode(client_socket, &bytecode, &routes_data);

    u32 message_size;
    u8 *message;
    if (return_code == INTPReturnCodeSuccess)
      message = create_message(return_code, bytecode.len, bytecode.ptr, &message_size);
    else
      message = create_message(return_code, 0, NULL, &message_size);

    send(client_socket, message, message_size, 0);

    free(message);
    close(client_socket);
  }

  close(server_socket);
  return 0;
}
