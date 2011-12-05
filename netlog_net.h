/**
 * The definition of both server and client to open the socket.
 * Copyright(C) 2011, <Shawn the R0ck, citypw@gmail.com>
 * 
 * The netlog is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * Version 3(GPLv3) as published by the Free Software Foundation.
 **/

#ifndef __NET_LOG_NET__
#define __NET_LOG_NET__

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <signal.h>
#include "netlog.h"

#define SA struct sockaddr

int open_netlog_socket(unsigned short srv_port, struct sockaddr_in* serv_addr)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		DEBUG_MSG("socket() error");
		return -1;
	}

	memset(serv_addr, 0x00, sizeof(struct sockaddr_in));

	serv_addr->sin_family = AF_INET;
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr->sin_port = htons(srv_port);

	if (bind(sockfd, (struct sockaddr *) serv_addr, sizeof(struct sockaddr_in)) < 0) {
		DEBUG_MSG("bind() error\n");
		return -1;
	}

	return sockfd;

}

int open_udp_client_socket(unsigned int cli_port, struct sockaddr_in *cli_addr, char *ip_addr)
{
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		DEBUG_MSG("socket() error");
		return -1;
	}

	memset(cli_addr, 0x00, sizeof(struct sockaddr_in));

	cli_addr->sin_family = AF_INET;
	inet_aton(ip_addr, cli_addr->sin_addr);
	cli_addr->sin_port = htons(cli_port);

	return sockfd;
}
#endif
