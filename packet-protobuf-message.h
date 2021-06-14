#ifndef PACKET_PROTOBUF_MESSAGE_H_
#define PACKET_PROTOBUF_MESSAGE_H_

#include <string>
#include <map>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif

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
	const std::string &forceMessage,
	const std::map<std::string, std::string> *tableAliases = NULL,
	const std::map<std::string, std::string> *fieldAliases = NULL
);

/**
 * Get message by type name
 * @param retMessage return message (NULL if fail)
 * @param env packet declaratuions
 * @param messageType,
 * @return false if fails
 */
bool getMessageByName(
	google::protobuf::Message **retMessage,
	void *env, 
	const std::string &messageType
);

#endif
