#ifndef RNET_H
#define RNET_H

#include <stdint.h>
#include <WinSock2.h>

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
// #define _WIN32_WINNT 0x0600
#include <WinSock2.h>
#include <ws2tcpip.h>

#else

// TODO Same thing as in rnet2.c. This WONT work on RoboRio (probably).
#define __USE_XOPEN2K

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif

enum NET_FAMILY {
	NET_AF_UNSPEC	  = AF_UNSPEC,
	NET_AF_UNIX		  = AF_UNIX,
	NET_AF_INET		  = AF_INET,
	NET_AF_IPX		  = AF_IPX,
	NET_AF_APPLETALK  = AF_APPLETALK,
	NET_AF_INET6	  = AF_INET6,
	NET_AF_DECnet	  = AF_DECnet,
	NET_AF_SNA		  = AF_SNA,
	NET_AF_IRDA		  = AF_IRDA  
};

enum NET_SOCKTYPE {
	// Sequenced, reliable, connection-based byte streams.
	NET_SOCK_STREAM = SOCK_STREAM,
	// Connectionless, unreliable datagrams of fixed maximum length.
	NET_SOCK_DGRAM = SOCK_DGRAM,
	// Raw protocol interface.
	NET_SOCK_RAW = SOCK_RAW,
	// Reliably-delivered messages.
	NET_SOCK_RDM = SOCK_RDM,
	// Sequenced, reliable, connection-based, datagrams of fixed maximum length.
	NET_SOCK_SEQPACKET = SOCK_SEQPACKET,
};

enum NET_PROTOCOL {
	// Dummy protocol for TCP.
	NET_IPPROTO_IP = IPPROTO_IP,
	// Internet Control Message Protocol.
	NET_IPPROTO_ICMP = IPPROTO_ICMP,
	// Internet Group Management Protocol.
	NET_IPPROTO_IGMP = IPPROTO_IGMP,
	// Transmission Control Protocol.
	NET_IPPROTO_TCP = IPPROTO_TCP,
	// Exterior Gateway Protocol.
	NET_IPPROTO_EGP = IPPROTO_EGP,
	// PUP protocol.
	NET_IPPROTO_PUP = IPPROTO_PUP,
	// User Datagram Protocol.
	NET_IPPROTO_UDP = IPPROTO_UDP,
	// XNS IDP protocol.
	NET_IPPROTO_IDP = IPPROTO_IDP,
	// IPv6 header.
	NET_IPPROTO_IPV6 = IPPROTO_IPV6,
	// encapsulating security payload.
	NET_IPPROTO_ESP = IPPROTO_ESP,
	// authentication header.
	NET_IPPROTO_AH = IPPROTO_AH,
	// Protocol Independent Multicast.
	NET_IPPROTO_PIM = IPPROTO_PIM,
	// Stream Control Transmission Protocol.
	NET_IPPROTO_SCTP = IPPROTO_SCTP,
	// Raw IP packets.
	NET_IPPROTO_RAW = IPPROTO_RAW,
	NET_IPPROTO_MAX	  = IPPROTO_MAX
};

enum NET_ADDRINFO_FLAGS {
	// Socket address is intended for `bind'.
	NET_AI_PASSIVE = AI_PASSIVE,
	// Request for canonical name.
	NET_AI_CANONNAME = AI_CANONNAME,
	// Don't use name resolution.
	NET_AI_NUMERICHOST = AI_NUMERICHOST,
	// IPv4 mapped addresses are acceptable.
	NET_AI_V4MAPPED = AI_V4MAPPED,
	// Return IPv4 mapped and IPv6 addresses.
	NET_AI_ALL = AI_ALL,
	// Use configuration of this host to choose returned address type.
	NET_AI_ADDRCONFIG = AI_ADDRCONFIG,
	// Don't use name resolution.
	NET_AI_NUMERICSERV = AI_NUMERICSERV
};

enum NET_ROLE {
	NET_ROLE_NONE,
	NET_ROLE_HOST,
	NET_ROLE_CLIENT
};

// Resolve errors.
enum NET_ERR_RESOLV {
	// NULL sock argument provided.
	NET_ERR_RESOLV_NULSOCK = -255,
	// Failed to get the address info.
	NET_ERR_RESOLV_ADDRINFO,
	// Failed to connect to the address.
	NET_ERR_RESOLV_CONN,
	// Faild to set the socket to non-blocking.
	NET_ERR_RESOLV_NOBLOCK,
};

// Hosting errors.
enum NET_ERR_HOST {
	// NULL sock argument provided.
	NET_ERR_HOST_NULSOCK = -255,
	// Failed to get the address info.
	NET_ERR_HOST_ADDRINFO,
	// Failed to connect to the address.
	NET_ERR_HOST_BIND,
	// Faild to set the socket to non-blocking.
	NET_ERR_HOST_NOBLOCK,
};

typedef struct net_socket_t net_socket_t;
typedef struct net_packet_t net_packet_t;

struct net_socket_t {
	int port;
	union addr {
		struct sockaddr		addr;
		struct sockaddr_in	ip4;
		struct sockaddr_in6 ip6;
	} addr;
	int addr_size;

	int family;
	int type;
	int protocol;
	int role;

	int fd;
};

int net_init();

/* -------------------------------------------------------------------------- */
/*                              Client functions                              */
/* -------------------------------------------------------------------------- */

// TODO Figure out if getaddrinfo resolves mdns address (both linux and
// windows). If it does, there is no reason for net_resolve_mdns so it can go
// away. Although, it might be nice to include our own implementation as well
// just for portibility. (see: https://github.com/mjansson/mdns)
// getaddrinfo uses the OS's DNS resolving meaning if avahi is installed on
// linux or bonjour on windows / osx, then it should work.
int net_resolve_mdns(char* addr, const int addr_size, net_socket_t* sock);
int net_resolve_ip(char* addr, const int addr_size, const int port,
				   net_socket_t* sock);

/* -------------------------------------------------------------------------- */
/*                               Host functions                               */
/* -------------------------------------------------------------------------- */

int net_host(const int port, net_socket_t* sock);
int net_host_mdns(char* addr, const int addr_size, const int port,
				  net_socket_t* sock);

/* -------------------------------------------------------------------------- */
/*                              Generic functions                             */
/* -------------------------------------------------------------------------- */

int net_send(net_socket_t* sock, uint8_t* data, const int data_size);
int net_receive(net_socket_t* sock, uint8_t* data, const int data_size);

int net_close(net_socket_t* sock);

#endif