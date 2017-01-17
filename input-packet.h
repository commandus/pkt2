#ifndef INPUT_PACKET_H
#define INPUT_PACKET_H     1

#include <sys/socket.h>

struct PacketHeader
{
    char name;
};

/**
  *      \see 
  */
class InputPacket
{
private:
    struct sockaddr socket_address_src;
    struct sockaddr socket_address_dst;
    void *buffer;
public:
    InputPacket(char typ, size_t data_size);

    /// buffer
    void *get();
    /// buffer size including address and header
    size_t size;
    
    struct PacketHeader* header();

    struct sockaddr_storage *get_socket_addr_src();
    struct sockaddr_storage *get_socket_addr_dst();
    
    void *data();
    /// max data buffer size
    int data_size;
    /// actual data length
    int length;

    int error();
};


#endif
