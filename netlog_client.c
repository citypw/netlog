/**
 * netlog_client is a sample test program that you can make your
 * own implementation. Just RTFSC....
 * Copyright(C) 2011, <Shawn the R0ck, citypw@gmail.com>
 * 
 * The netlog is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * Version 3(GPLv3) as published by the Free Software Foundation.
 **/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include "netlog.h"


int sockfd, addrlen;
struct sockaddr_in cli_addr;
struct netlog_packet n;
char *ip_addr = "127.0.0.1";
u32 cli_port = 9046;

void netlog_init()
{
  addrlen = sizeof(struct sockaddr_in);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0){
    DEBUG_MSG("socket() error!\n");
    exit(1);
  }

  memset(&cli_addr, 0x00, sizeof(struct sockaddr_in));

  cli_addr.sin_family = AF_INET;
  inet_aton(ip_addr, cli_addr.sin_addr);
  cli_addr.sin_port = htons(cli_port);

}

void netlog(u16 type, char *msg)
{

  netlog_init();
  struct netlog_packet np;

  np.type = type;
  if(strlen(msg) >= MAX_MSG_SIZE){
    strncpy(np.msg, msg, MAX_MSG_SIZE);
  }else{
    strcpy(np.msg, msg);
  }

  sendto(sockfd, &np, sizeof(np), 0, (struct sockaddr*)&cli_addr, addrlen);

  close(sockfd);
}
int main(int argc, char *argv[])
{
  netlog(LOG_INFO, "this is test");
  return 0; 

}
