#ifndef PACKET_PROTOBUF_MESSAGE_H_
#define PACKET_PROTOBUF_MESSAGE_H_

#include <string>
#include <netinet/in.h>

#include <google/protobuf/message.h>

/*
 * @file packet-protobuf-message.h
 */

/**
 * Parse packet by declaration
 * @param retMessage return message
 * @param env packet declaratuions
 * @param inputFormat 0- binary, 1- hex string
 * @param packet data
 * @param forceMessage "" If specifed, try only message type
 * @return empty string if fails
 */
bool parsePacket2Message(
	google::protobuf::Message **retMessage,
	void *env, 
	int inputFormat,
	const std::string &packet,
	const std::string &forceMessage
);

#endif
