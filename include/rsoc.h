// TODO rename rnet to rsock or rskt to go better with rhid

#ifndef RSOC_H
#define RSOC_H

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

enum rsoc_family_t {
	RSOC_AF_UNSPEC	  = AF_UNSPEC,
	RSOC_AF_UNIX	  = AF_UNIX,
	RSOC_AF_INET	  = AF_INET,
	RSOC_AF_IPX		  = AF_IPX,
	RSOC_AF_APPLETALK = AF_APPLETALK,
	RSOC_AF_INET6	  = AF_INET6,
	RSOC_AF_DECnet	  = AF_DECnet,
	RSOC_AF_SNA		  = AF_SNA,
	RSOC_AF_IRDA	  = AF_IRDA
};

enum rsoc_soctype_t {
	// Sequenced, reliable, connection-based byte streams.
	RSOC_SOCK_STREAM = SOCK_STREAM,
	// Connectionless, unreliable datagrams of fixed maximum length.
	RSOC_SOCK_DGRAM = SOCK_DGRAM,
	// Raw protocol interface.
	RSOC_SOCK_RAW = SOCK_RAW,
	// Reliably-delivered messages.
	RSOC_SOCK_RDM = SOCK_RDM,
	// Sequenced, reliable, connection-based, datagrams of fixed maximum length.
	RSOC_SOCK_SEQPACKET = SOCK_SEQPACKET,
};

enum RSOC_PROTOCOL {
	// Dummy protocol for TCP.
	RSOC_IPPROTO_IP = IPPROTO_IP,
	// Internet Control Message Protocol.
	RSOC_IPPROTO_ICMP = IPPROTO_ICMP,
	// Internet Group Management Protocol.
	RSOC_IPPROTO_IGMP = IPPROTO_IGMP,
	// Transmission Control Protocol.
	RSOC_IPPROTO_TCP = IPPROTO_TCP,
	// Exterior Gateway Protocol.
	RSOC_IPPROTO_EGP = IPPROTO_EGP,
	// PUP protocol.
	RSOC_IPPROTO_PUP = IPPROTO_PUP,
	// User Datagram Protocol.
	RSOC_IPPROTO_UDP = IPPROTO_UDP,
	// XNS IDP protocol.
	RSOC_IPPROTO_IDP = IPPROTO_IDP,
	// IPv6 header.
	RSOC_IPPROTO_IPV6 = IPPROTO_IPV6,
	// encapsulating security payload.
	RSOC_IPPROTO_ESP = IPPROTO_ESP,
	// authentication header.
	RSOC_IPPROTO_AH = IPPROTO_AH,
	// Protocol Independent Multicast.
	RSOC_IPPROTO_PIM = IPPROTO_PIM,
	// Stream Control Transmission Protocol.
	RSOC_IPPROTO_SCTP = IPPROTO_SCTP,
	// Raw IP packets.
	RSOC_IPPROTO_RAW = IPPROTO_RAW,
	RSOC_IPPROTO_MAX = IPPROTO_MAX
};

enum RSOC_ADDRINFO_FLAGS {
	// Socket address is intended for `bind'.
	RSOC_AI_PASSIVE = AI_PASSIVE,
	// Request for canonical name.
	RSOC_AI_CANONNAME = AI_CANONNAME,
	// Don't use name resolution.
	RSOC_AI_NUMERICHOST = AI_NUMERICHOST,
	// IPv4 mapped addresses are acceptable.
	RSOC_AI_V4MAPPED = AI_V4MAPPED,
	// Return IPv4 mapped and IPv6 addresses.
	RSOC_AI_ALL = AI_ALL,
	// Use configuration of this host to choose returned address type.
	RSOC_AI_ADDRCONFIG = AI_ADDRCONFIG,
	// Don't use name resolution.
	RSOC_AI_NUMERICSERV = AI_NUMERICSERV
};

enum RSOC_ROLE { RSOC_ROLE_NONE, RSOC_ROLE_HOST, RSOC_ROLE_CLIENT };

// Resolve errors.
enum RSOC_ERR_RESOLV {
	// NULL sock argument provided.
	RSOC_ERR_RESOLV_NULSOCK = -255,
	// Failed to get the address info.
	RSOC_ERR_RESOLV_ADDRINFO,
	// Failed to connect to the address.
	RSOC_ERR_RESOLV_CONN,
	// Faild to set the socket to non-blocking.
	RSOC_ERR_RESOLV_NOBLOCK,
};

// Hosting errors.
enum RSOC_ERR_HOST {
	// NULL sock argument provided.
	RSOC_ERR_HOST_NULSOCK = -255,
	// Failed to get the address info.
	RSOC_ERR_HOST_ADDRINFO,
	// Failed to connect to the address.
	RSOC_ERR_HOST_BIND,
	// Faild to set the socket to non-blocking.
	RSOC_ERR_HOST_NOBLOCK,
};

typedef struct rsoc_socket_t rsoc_socket_t;
typedef struct rsoc_packet_t rsoc_packet_t;

struct rsoc_socket_t {
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

int rsoc_init();

// CLIENT FUNCTIONS

// TODO Figure out if getaddrinfo resolves mdns address (both linux and
// windows). If it does, there is no reason for rsoc_resolve_mdns so it can go
// away. Although, it might be nice to include our own implementation as well
// just for portibility. (see: https://github.com/mjansson/mdns)
// getaddrinfo uses the OS's DNS resolving meaning if avahi is installed on
// linux or bonjour on windows / osx, then it should work.
int rsoc_resolve_mdns(char* addr, const int addr_size, rsoc_socket_t* sock);
int rsoc_resolve_ip(char* addr, const int addr_size, const int port,
				   rsoc_socket_t* sock);

// HOST FUNCTIONS

int rsoc_host(const int port, rsoc_socket_t* sock);
int rsoc_host_mdns(char* addr, const int addr_size, const int port,
				  rsoc_socket_t* sock);

// GENERIC FUNCTIONS

int rsoc_send(rsoc_socket_t* sock, uint8_t* data, const int data_size);
int rsoc_receive(rsoc_socket_t* sock, uint8_t* data, const int data_size);

int rsoc_close(rsoc_socket_t* sock);

#endif