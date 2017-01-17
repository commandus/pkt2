#ifndef INPUT_PACKET_H
#define INPUT_PACKET_H     1

#include <sys/socket.h>

#define PROGRAM_NAME             "tcpreceiver"
#define PROGRAM_DESCRIPTION      "PKT2 tcp packet listener"

/**
  *      \see 
  */
class InputPacket
{
private:
    struct sockaddr socket_address;
    void *buffer;
public:
    InputPacket(size_t data_size);
    /// buffer
    void *get();
    /// buffer size
    size_t size;
    
    struct sockaddr_storage *get_socket_addr();
    size_t addr_size;

    void *data();
    /// data length
    int data_size;
    int length;

    int error();
};


#endif
