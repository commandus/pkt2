#include "input-packet.h"

#include <cstdlib>
#include <string.h>

#include "errorcodes.h"

InputPacket::InputPacket(
    char typ,
    size_t adata_size
) 
{
    data_size = adata_size;
    size = sizeof(struct PacketHeader) + data_size + sizeof(struct sockaddr_storage) + sizeof(struct sockaddr_storage);
    buffer = malloc(size);
    if (buffer)
    {
        header()->name = typ;
        allocated = true;
    }
    else
    {
        allocated = false;
    }
}

InputPacket::InputPacket
(
    void *data,
    size_t adata_size
) 
{
    buffer = data;
    size = adata_size;
    allocated = false;
    length = 0;
    data_size = 0;
    parse();
}

InputPacket::~InputPacket() 
{
    if (message)
    	delete message;
    if (allocated && (buffer != NULL))
        free(buffer);
}

void *InputPacket::get()
{
    return buffer;
}

struct PacketHeader* InputPacket::header()
{
    return (PacketHeader*) buffer;
}

struct sockaddr_storage *InputPacket::get_socket_addr_src()
{
    return (struct sockaddr_storage *) buffer + sizeof(struct PacketHeader);
}

struct sockaddr_storage *InputPacket::get_socket_addr_dst()
{
    return (struct sockaddr_storage *) buffer + sizeof(struct PacketHeader) + sizeof(struct sockaddr_storage);
}

void *InputPacket::data()
{
    return (char*) buffer + sizeof(struct PacketHeader) + sizeof(struct sockaddr_storage) + sizeof(struct sockaddr_storage);
}

bool InputPacket::parse()
{
	// TODO
	// 1. message
	google::protobuf::Message *r = NULL;
	void *d = data();
	if (!d)
		return false;

	// 2. key
	memset(&key, sizeof(key), 0);

	return true;
}

/**
  * @return 0- success
  *        1- no enough memory
  * @see errorcode.h
  */
int InputPacket::error()
{
    if (!buffer)
        return ERRCODE_PACKET_PARSE;
    return ERR_OK;
}
