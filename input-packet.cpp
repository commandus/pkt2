#include "input-packet.h"

#include <cstdlib>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <WS2tcpip.h>

#else
#include <netinet/in.h>
#include <netdb.h>
#endif

#include <iostream>

#include "errorcodes.h"

InputPacket::InputPacket(
    char typ,
    size_t adata_size
) 
	: message(NULL)
{
    max_data_size = adata_size;
    size = sizeof(struct PacketHeader) + (2 * sizeof(struct sockaddr_storage)) + max_data_size;
    length = 0;
    buffer = malloc(size);
    memset(buffer, 0, size);
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
	: message(NULL)
{
    buffer = data;
    size = adata_size;
    allocated = false;
    length = size - sizeof(struct PacketHeader) - (2 * sizeof(struct sockaddr_storage));
    max_data_size = length;
    parse();
}

InputPacket::InputPacket
(
	const std::string &data
)
	: message(NULL)
{
    buffer = (void *) &data[0];
    size = data.size();
    allocated = false;
    length = size - sizeof(struct PacketHeader) - (2 * sizeof(struct sockaddr_storage));
    max_data_size = length;
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

struct sockaddr_in *InputPacket::get_sockaddr_src()
{
	return (sockaddr_in *) get_socket_addr_src();

}

struct sockaddr_in *InputPacket::get_sockaddr_dst()
{
	return (sockaddr_in *) get_socket_addr_dst();
}

bool InputPacket::set_socket_addr_src
(
	struct addrinfo *value
)
{
	memmove(get_socket_addr_src(), value->ai_addr, sizeof(struct sockaddr));
	return true;
}

bool InputPacket::set_socket_addr_dst
(
	struct addrinfo *value
)
{
	memmove(get_socket_addr_dst(), value->ai_addr, sizeof(struct sockaddr));
	return true;
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
	memset(&key, 0, sizeof(key));

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

void InputPacket::setLength(int value)
{
	length = value;
	size = value + sizeof(struct PacketHeader) + (2 * sizeof(struct sockaddr_storage));
}

/**
	* @brief return payload size
	* @return payload size
	*/
int InputPacket::getPayloadSize
(
	int full_packet_size
)
{
	return full_packet_size - sizeof(struct PacketHeader) - (2 * sizeof(struct sockaddr_storage));
}
