#include "utilinet.h"

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <WS2tcpip.h>

struct ifaddrs {
    struct ifaddrs* ifa_next;    /* Next item in list */
    char* ifa_name;    /* Name of interface */
    unsigned int     ifa_flags;   /* Flags from SIOCGIFFLAGS */
    struct sockaddr* ifa_addr;    /* Address of interface */
    struct sockaddr* ifa_netmask; /* Netmask of interface */

    struct sockaddr_storage in_addrs;
    struct sockaddr_storage in_netmasks;

    char		   ad_name[16];
    size_t		   speed;
};

void getifaddrs(struct ifaddrs** ifAddrStruct)
{
        // TODO
}
#else
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

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
            std::string iface(ifa->ifa_name);
            if (iface == "lo")
            	continue;
            switch (range)
            {
            case IR_IPV4_BROADCAST:
				{
					uint32_t m = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
					uint32_t g = ((sockaddr_in *) ifa->ifa_netmask)->sin_addr.s_addr;
					struct in_addr a;
					a.s_addr = m | (~g);
					r[iface] = ipv4ToString(a);
				}
            	break;
            default:
            	r[iface] = std::string(ipv4ToString(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
            	break;
            }
        }
    }
    return r;
}
