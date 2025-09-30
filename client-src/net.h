#ifndef NET_H
#define NET_H

#include "shl_defs.h"
#include "shl_str.h"

Str get_bytecode_from_server(char *address, u32 port, char *route);

#endif // NET_H
