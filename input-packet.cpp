#include "input-packet.h"

#include <cstdlib>

InputPacket::InputPacket(
    size_t adata_size
) 
{
    data_size = adata_size;
    addr_size = sizeof(struct sockaddr_storage);
    size = data_size + addr_size;
    buffer = malloc(size);
}

void *InputPacket::get()
{
    return buffer;
}

struct sockaddr_storage *InputPacket::get_socket_addr()
{
    return (struct sockaddr_storage *) buffer;
}

void *InputPacket::data()
{
    return (char*) buffer + addr_size;
}

/**
  * Return 0- success
  *        1- no enought memory
  */
int InputPacket::error()
{
    if (!buffer)
        return 1;
    return 0;
}

