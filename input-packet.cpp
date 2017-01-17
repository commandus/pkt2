#include "input-packet.h"

InputPacket::InputPacket(
    void *buf, 
    size_t len
) : data(buf), size(len)
{
}

InputPacket::InputPacket(
    struct sockaddr *socket_address, 
    void *buffer, 
    size_t size
) : InputPacket(buffer, size)
{
    
}

