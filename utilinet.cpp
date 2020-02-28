#include "utilinet.h"

#include <string.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static std::string ipv4ToString(struct in_addr value)
{
    char address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &value, address, INET_ADDRSTRLEN);
    return std::string(address);
}

/**
 * Return list of interface IPv4 addresses
 */
std::map<std::string, std::string> pkt2utilinet::getIP4Addresses(enum IPADDRRANGE range)
{
	std::map<std::string, std::string> r;
	struct ifaddrs *ifAddrStruct;
	getifaddrs(&ifAddrStruct);
    for (struct ifaddrs *ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            std::string interface(ifa->ifa_name);
            if (interface == "lo")
            	continue;
            switch (range)
            {
            case IR_IPV4_BROADCAST:
				{
					uint32_t m = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
					uint32_t g = ((sockaddr_in *) ifa->ifa_netmask)->sin_addr.s_addr;
					struct in_addr a;
					a.s_addr = m | (~g);
					r[interface] = ipv4ToString(a);
				}
            	break;
            default:
            	r[interface] = std::string(ipv4ToString(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
            	break;
            }
        }
    }
    return r;
}
