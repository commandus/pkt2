#ifndef INPUT_PACKET_H
#define INPUT_PACKET_H     1

#include <string>

#define PROGRAM_NAME             "tcpreceiver"
#define PROGRAM_DESCRIPTION      "PKT2 tcp packet listener"

/**
  *      \see 
  */
class InputPacket
{
private:
public:
    InputPacket(void *buffer, size_t len);
    InputPacket(struct sockaddr * socket_address, void *buffer, size_t size);

    void *data;
    size_t size;
};


#endif
