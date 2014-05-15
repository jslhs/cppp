#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32")

#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

using SOCKET = int;
#define INVALID_SOCKET (SOCKET)-1
#define SOCKET_ERROR -1

#endif

#include <string>

namespace net
{
	static const std::string any_addr = "0.0.0.0";
	static const std::string any_addr_v6 = "::";
	static const std::string loopback_addr = "127.0.0.1";
	static const std::string loopback_addr_v6 = "::1";

	template<int AF, int TYPE, int PROTO>
	class socket_t
	{
	public:
		socket_t();
		socket_t(const socket_t &) = delete;
		socket_t(socket_t &&s);
		~socket_t();

		socket_t &operator=(const socket_t &) = delete;
		socket_t &operator=(socket_t &&s);

		bool operator==(const socket_t &s) const;
		bool operator!=(const socket_t &s) const;

		bool bind(const std::string &addr, int port);
		bool connect(const std::string &addr, int port);
		int send();
		int recv();
		bool listen(int backlog = SOMAXCONN);
		void close();
		void shutdown();
		bool ioctl();
		void setopt();
		int opt() const;
		socket_t accept();
		std::string name() const;

		operator SOCKET() const;

	private:
		SOCKET _sock;
	};

	template<class It_Read_Sock, class It_Write_Sock, class It_Except_Sock>
	int select(It_Read_Sock first_read_fds
		, It_Read_Sock last_read_sock_fds
		, It_Write_Sock first_write_fds = nullptr
		, It_Write_Sock last_write_fds = nullptr
		, It_Except_Sock first_except_fds = nullptr
		, It_Except_Sock last_except_fds = nullptr);

	using tcp_socket = socket_t<AF_INET, SOCK_STREAM, IPPROTO_TCP>;
	using udp_socket = socket_t<AF_INET, SOCK_DGRAM, IPPROTO_UDP>;
	using tcp_socket_v6 = socket_t<AF_INET6, SOCK_STREAM, IPPROTO_TCP>;
	using udp_socket_v6 = socket_t<AF_INET6, SOCK_DGRAM, IPPROTO_UDP>;
}

#include "socket_impl.hpp"
