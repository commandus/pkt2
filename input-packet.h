#ifndef INPUT_PACKET_H
#define INPUT_PACKET_H     1

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>

#else
#include <sys/socket.h>
#endif

#include <google/protobuf/message.h>
#include "output-message.h"

struct PacketHeader
{
    char name;
};

/**
 * @brief nanomsg packet
 */
class InputPacket
{
private:
    struct sockaddr socket_address_src;
    struct sockaddr socket_address_dst;
    void *buffer;
    bool allocated;			///< true- buffer is malloc'ed, false- external buffer
    bool parse();
public:
    struct OutputMessageKey key;
    google::protobuf::Message *message;

    InputPacket(char typ, size_t data_size);
    InputPacket(void *data, size_t data_size);
    InputPacket(const std::string &data);
    virtual ~InputPacket();

    /// buffer
    void *get();
    /// buffer size including address and header
    size_t size;
    
    struct PacketHeader* header();

    struct sockaddr_storage *get_socket_addr_src();
    struct sockaddr_storage *get_socket_addr_dst();
    struct sockaddr_in *get_sockaddr_src();
    struct sockaddr_in *get_sockaddr_dst();
    
    bool set_socket_addr_src(struct addrinfo *value);
    bool set_socket_addr_dst(struct addrinfo *value);

    void *data();
    /// max data buffer size
    int max_data_size;
    /// actual data length
    int length;
    void setLength(int length);
    int error();
	/**
	 * @brief return payload size
	 * @return payload size
	 */
	static int getPayloadSize(int full_packet_size);
};

#endif
