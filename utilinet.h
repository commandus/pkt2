/*
 * utilinet.h
 *
 *  Created on: Jun 7, 2016
 *      Author: andrei
 */

#ifndef UTILINET_H_
#define UTILINET_H_

#include <map>
#include <string>

enum IPADDRRANGE
{
	IR_IPV4 = 0,
	IR_IPV4_BROADCAST = 1
};
/**
 * Return list of interface IPv4 addresses
 */
std::map<std::string, std::string> getIP4Addresses(enum IPADDRRANGE range);

#endif /* UTILINET_H_ */
