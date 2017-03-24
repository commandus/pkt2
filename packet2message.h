/*
 * packet2message.h
 *
 */

#ifndef PACKET2MESSAGE_H_
#define PACKET2MESSAGE_H_

#include <google/protobuf/message.h>

class Packet2Message {
public:
	Packet2Message();
	virtual ~Packet2Message();
	static google::protobuf::Message *parse(const std::string &protopath, const std::string &packet);
};

#endif /* PACKET2MESSAGE_H_ */
