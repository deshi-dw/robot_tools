#include "rsoc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <winsock.h>

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS
// #define _WIN32_WINNT 0x0600
#include <WinSock2.h>
#include <ws2tcpip.h>

#else

// TODO This wont work on the RoboRIO. I don't know what will so look into it
// yourself idiot: https://github.com/ni/linux
#define __USE_XOPEN2K

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif

#define RSOC_ERR(message) fprintf(stderr, message);
#define RSOC_ERR_SOCK(message, sockerr)                  \
	{                                                    \
		char errmsg[256];                                \
		strerror_s(errmsg, sizeof(errmsg), sockerr);     \
		fprintf(stderr, message " error: %s\n", errmsg); \
	}

// TODO look into using select(2) and poll(2) functions.
// These two functions monitor file descriptors until they are ready for I/O
// operations.

int rsoc_init() {
	WSADATA data;
	if(WSAStartup(0, &data) != 0) {
		return -1;
	}

	return 0;
}

// FIXME function doesn't work with TCP. See
// https://linux.die.net/man/2/accept
static int _rsoc_conn(char* addr, const int addr_size, const int port,
					  rsoc_socket_t* sock, int role) {
	if(sock == NULL) {
		return RSOC_ERR_RESOLV_NULSOCK;
	}

	if(role != RSOC_ROLE_HOST && role != RSOC_ROLE_CLIENT) {
		return -1;
	}

	int ret;

	struct addrinfo hints = {0};
	hints.ai_family		  = sock->family;
	hints.ai_socktype	  = sock->type;
	hints.ai_flags		  = RSOC_AI_PASSIVE;
	hints.ai_protocol	  = sock->protocol;

	// convert the numerical port value into a string value.
	char port_str[6];
	snprintf(port_str, 6, "%i", port);

	// get a list of addrinfo structs on the supplied port of this machine.
	struct addrinfo* info_list;
	ret = getaddrinfo(addr, port_str, &hints, &info_list);
	if(ret < 0) {
		return RSOC_ERR_RESOLV_ADDRINFO;
	}

	// start the loop again but this time fill the list.
	struct addrinfo* info_curr = info_list;
	do {
		// try to create a socket with the address parameters given by the
		// resolve call.
		sock->fd = socket(info_curr->ai_family, info_curr->ai_socktype,
						  info_curr->ai_protocol);
		if(sock->fd < 0) {
			info_curr = info_curr->ai_next;
			continue;
		}

		if(role == RSOC_ROLE_CLIENT) {
			// try to connect the socket to the host. if this fails, error out.
			if(connect(sock->fd, info_curr->ai_addr, info_curr->ai_addrlen) <
			   0) {
#ifdef _WIN32
				closesocket(sock->fd);
#else
				close(sock->fd);
#endif
			}
			freeaddrinfo(info_list);
			return RSOC_ERR_RESOLV_CONN;
		}
		else if(role == RSOC_ROLE_HOST) {
			// if the socket was created successfully, bind it to the address so
			// we can use it as a host.
			if(bind(sock->fd, info_curr->ai_addr, info_curr->ai_addrlen) < 0) {
#ifdef _WIN32
				closesocket(sock->fd);
#else
				close(sock->fd);
#endif
				freeaddrinfo(info_list);
				return RSOC_ERR_HOST_BIND;
			}

			// non-blocking socket disabled here because it is more convinient
			// if it actually does block. If this changes just uncomment the
			// code below.
#ifdef _WIN32
			// int opt = 1;
			// ret = ioctlsocket(sockfd, FIONBIO, (u_long*) (&opt));
#else
			// int opt = 1;
			// ret = ioctl(sock->fd, FIONBIO, &opt);
#endif
			if(ret != 0) {
				freeaddrinfo(info_list);
				return RSOC_ERR_RESOLV_NOBLOCK;
			}

			sock->addr.addr = *info_curr->ai_addr;
			sock->addr_size = info_curr->ai_addrlen;
			// TODO consider using htons(port) here.
			sock->port = port;

			sock->family   = info_curr->ai_family;
			sock->type	   = info_curr->ai_socktype;
			sock->protocol = info_curr->ai_protocol;
			sock->role	   = RSOC_ROLE_CLIENT;

			break;
		}
	} while(info_curr != NULL);

	return 0;
}

// CLIENT FUNCTIONS

int rsoc_resolve_mdns(char* addr, const int addr_size, rsoc_socket_t* socket) {
	return 0;
}

int rsoc_resolve_ip(char* addr, const int addr_size, const int port,
					rsoc_socket_t* sock) {
	int ret = _rsoc_conn(addr, addr_size, port, sock, RSOC_ROLE_CLIENT);
	if(ret < 0) {
		return ret;
	}

	return 0;
}

// HOST FUNCTIONS

int rsoc_host(const int port, rsoc_socket_t* sock) {
	int ret = _rsoc_conn(NULL, 0, port, sock, RSOC_ROLE_HOST);
	if(ret < 0) {
		return ret;
	}

	return 0;
}

int rsoc_host_mdns(char* addr, const int addr_size, const int port,
				   rsoc_socket_t* sock) {
	return -1;
}

// GENERIC FUNCTIONS

int rsoc_send(rsoc_socket_t* sock, uint8_t* data, const int data_size) {
	if(sock->role == RSOC_ROLE_NONE) {
		return -1;
	}

	int send_size = 0;

	// send the data to whatever address is saved in sock if this is a host
	// or to whatever address the socket is connected if this is a client.
	if(sock->role == RSOC_ROLE_HOST) {
		if(sock->addr_size <= 0) {
			return -1;
		}

		send_size = sendto(sock->fd, (const char*) data, data_size, 0,
						   &sock->addr.addr, sock->addr_size);
	}
	else if(sock->role == RSOC_ROLE_CLIENT) {
		send_size = send(sock->fd, (const char*) data, data_size, 0);
	}

	// error out if nothing was sent or an error occured.
	if(send_size <= 0) {
		RSOC_ERR_SOCK("didn't send any data in rsoc_send", errno);
		return -1;
	}

	return send_size;
}

static int rsoc_receivefrom(rsoc_socket_t* sock, uint8_t* data,
							const int data_size, int flags) {
	if(sock->role == RSOC_ROLE_NONE) {
		return -1;
	}

	int recv_size = 0;

	if(sock->role == RSOC_ROLE_HOST) {
		struct sockaddr_storage addr	  = {0};
		int						addr_size = sizeof(struct sockaddr_storage);

		recv_size = recvfrom(sock->fd, (char*) data, data_size, flags,
							 (struct sockaddr*) &addr, (socklen_t*) &addr_size);
		// recv_size = recvfrom(sock->fd, data, data_size, 0, NULL, NULL);

		if(recv_size <= 0) {
			RSOC_ERR_SOCK(
				"didn't recieve any data as host when calling rsoc_receive",
				errno);
			return -1;
		}

		// copy the received address into the sock address.
		sock->addr_size = addr_size;
		if(addr_size == sizeof(struct sockaddr_in)) {
			sock->addr.ip4 = *(struct sockaddr_in*) &addr;
		}
		else if(addr_size == sizeof(struct sockaddr_in6)) {
			sock->addr.ip6 = *(struct sockaddr_in6*) &addr;
		}
		else {
			sock->addr_size = 0;
			return -1;
		}
	}
	else if(sock->role == RSOC_ROLE_CLIENT) {
		// receive data from whatever host we're connected to.
		recv_size = recv(sock->fd, (char*) data, data_size, flags);

		if(recv_size <= 0) {
			RSOC_ERR_SOCK(
				"didn't recieve any data as client when calling rsoc_receive",
				errno);
			return -1;
		}
	}

	return recv_size;
}

int rsoc_receive(rsoc_socket_t* sock, uint8_t* data, const int data_size) {
	return rsoc_receivefrom(sock, data, data_size, 0);
}

int rsoc_peek(rsoc_socket_t* sock, uint8_t* data, const int data_size) {
	return rsoc_receivefrom(sock, data, data_size, MSG_PEEK);
}

int rsoc_close(rsoc_socket_t* sock) {
#ifdef _WIN32
	int ret = closesocket(sock->fd);
#else
	int ret = close(sock->fd);
#endif
	if(ret < 0) {
		return -1;
	}

	sock->role = RSOC_ROLE_NONE;

	return 0;
}
