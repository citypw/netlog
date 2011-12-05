/**
 * netlog is a network server program which handles log record from client-end
 * Copyright(C) 2011, <Shawn the R0ck, citypw@gmail.com>
 * 
 * The netlog is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * Version 3(GPLv3) as published by the Free Software Foundation.
 **/

#include <stdio.h> /* standard input and ouput */
#include <signal.h> /* handle signals */
#include "netlog.h"
#include "netlog_net.h"
#include "netlog_thread_pool.h"

/* for directory functions, such as opendir().... */
#include <sys/types.h>
#include <dirent.h>
/* for getopt() and getopt_long() series functions */ 
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h> /* exit() */
#include <sys/socket.h> /* socket() */
#include <sys/ioctl.h> /* ioctl() */
#include <string.h> /* memset() */
#include <linux/if.h> /* struct ifr */
#include <netinet/in.h> /* struct in_addr contain only a u32 */
#include <time.h> /* time() */

u32 glb_fd;
struct netlog_conf log_conf;
FILE *netlog_fp;
char netlog_buf[MAX_BUF_SIZE];
char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

struct option longopts[] = {
    {"debug", LONGOPT_ARG_NONE, NULL, 'D'},
    {"help", LONGOPT_ARG_NONE, NULL, 'H'},
    {"version", LONGOPT_ARG_OPTIONAL, NULL, 'V'},
    {"conf", LONGOPT_ARG_REQUIRED, NULL, 'C'},
    {"intro", LONGOPT_ARG_REQUIRED, NULL, 'I'}
};

/**
 * $netlog_parse_cmd - use long options to parsing the input from command line
 * @argc, it's a pointer to integer, which is the number of arguments
 * @argv, it's a string vector
 * */
void netlog_parse_cmd(int *argc, char *argv[])
{
    int ch;
    char *cp;
    FILE *fp;

    if (*argc < 2)
	goto netlog_help;

    while ((ch = getopt_long(*argc, argv, valid_opts, longopts, NULL)) != -1) {
	switch (ch) {
	case 'D':
	  printf("Starting netlogd as daemon\n");
	  DAEMON_ON = 1;
	    break;
	case 'H':
	  netlog_help:
	    printf("Usage:%s [OPTION] [URL]\nShort options:\n   -H, --help\n \
  -V, --version\n   -D, --daemon   Run the program as daemon\n   -C, --conf     config file path\n   Example: %s -C \
/root/hello.conf\n",
		 argv[0], argv[0]);
	    exit(1);
	case 'V':
	  cp = optarg;
	  printf("netlogd v0.1. Monkey-coders: Shawn the R0ck@copyleft, <citypw@gmail.com>\n");
	    break;
	case 'C':
	  cp = optarg;
	  printf("Config file path: %s\n", cp);
	  fp = fopen(cp, "r");
	  if(fp == NULL){
	    DEBUG_MSG("fp is null!\n");
	    exit(1);
	  }

	  log_conf = netlog_read_ini(fp);
	  if(netlog_flush_init() != 0)
	    exit(1);
	  break;
	case 'I':
	  cp = optarg;
	  if(strncmp(cp, "shawn", 5) == 0 )
	    printf("add here later!\n");
	  break;
	}
    }
}

/**
 * $init_daemon - initlizing the netlog as a daemon server
 */
void netlog_init_daemon()
{
    int pid;
    int i;
    int max_fd = 64;

    if ((pid = fork()) != 0)
	exit(0);

    setsid();
    signal(SIGHUP, SIG_IGN);

    if ((pid = fork()) != 0)
	exit(0);

    for (i = 0; i < max_fd; i++)
	close(i);

    chdir("/");
    umask(0);

}


/**
 * $check_instanc - check whether the program have a running instance or not
 *
 * if there's a instance exist, return 0. Otherwise, return 1.
 */
int netlog_check_instance()
{
    FILE *fp;
    DIR *dir;
    char proc_buf[64] = { 0 };
    unsigned int pid = 0;

    fp = fopen(PID_FILE, "r");
    if (fp == NULL) {
	DEBUG_MSG("fp is null\n");
	return 1;
    }

    fscanf(fp, "%u\n", &pid);
    fclose(fp);

    sprintf(proc_buf, "/proc/%d", pid);

    if ((dir = opendir(proc_buf)) == NULL) {
	DEBUG_MSG("No running instace now! continue to run...\n");
	return 1;
    }

    return 0;
}

void netlog_write_pid()
{
    FILE *fp = NULL;

    if ((fp = fopen(PID_FILE, "w")) == NULL) {
	DEBUG_MSG("fp is null!\n");
	exit(1);
    }

    fprintf(fp, "%u\n", getpid());
    fclose(fp);
}

struct netlog_conf netlog_read_ini(FILE *fp)
{
    struct netlog_conf log_conf;
    char buf[256], *cp;
    memset(buf, '\0', 256);
    memset(&log_conf, 0x00, sizeof(struct netlog_conf));

    while (fgets(buf, sizeof(buf), fp) != NULL) {
	cp = strtok(buf, "=");

	if (strcmp(cp, "netlog_path") == 0) {
	    cp = strtok(NULL, "=");
	    strncpy(log_conf.netlog_path, cp, strlen(cp));
	    log_conf.netlog_path[ strlen(log_conf.netlog_path) - 1] = '\0';
	    DEBUG_ERR("NETLOG PATH: %s\n", log_conf.netlog_path);
	    continue;
	}

	if (strcmp(cp, "netlog_port") == 0) {
	    cp = strtok(NULL, "=");
	    log_conf.netlog_port = atoi(cp);
	    DEBUG_ERR("NETLOG PORT:%d\n", log_conf.netlog_port);
	    continue;
	}

	if(strcmp(cp, "netlog_threads") == 0){
	  cp = strtok(NULL, "=");
	  log_conf.threads_num = atoi(cp);
	  DEBUG_ERR("NETLOG THREADS NUM: %d\n", log_conf.threads_num);
	  continue;
	}

	memset(buf, '\0', 256);
    }
    fclose(fp);

    return log_conf;
}

u32 get_tick_count()
{
    FILE *fp;
    char buf[256];
    u32 secs = 0;

    fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
	DEBUG_MSG("fp is null!\n");
	return -1;
    }

    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%d", &secs);

    return secs;
}

unsigned long get_ipaddr(char *dev)
{
    struct ifreq ifr;
    int fd;
    unsigned long ip_addr;
    struct in_addr tmp_addr;

    strcpy(ifr.ifr_name, dev);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(fd, SIOCGIFADDR, &ifr)) {
	return 1;
    }

    close(fd);
    memcpy(&ip_addr, ifr.ifr_addr.sa_data + 2, 4);
    tmp_addr.s_addr = ip_addr;

    return ip_addr;
}

void handle_int_signal(int signo)
{
  DEBUG_MSG("The program has been interrupted by No.%d!\n", signo);
  close(glb_fd);
  fclose(netlog_fp);
  exit(1);
}

/**
 * init_signals - register some signals which might interrupt the execution
 */
void netlog_init_signals()
{
    signal(SIGTERM, handle_int_signal);
    signal(SIGINT, handle_int_signal);
    signal(SIGHUP, handle_int_signal);
}

struct netlog_packet log_packet[MAX_PACKETS_NUM];
u32 packet_count = 0;
struct sockaddr_in netlog_addr;
u32 addrlen;

/**
 * netlog_flush_init - initializing the file pointer
 * return 0, if succeed; return 1, if fails
 */
u16 netlog_flush_init()
{
  netlog_fp = fopen(log_conf.netlog_path, "w+");
  DEBUG_MSG("%s\n", log_conf.netlog_path);
  if(netlog_fp == NULL)
    {
      DEBUG_MSG("netlog_fp is null!\n");
      return 1;
    }

  return 0;
}

void netlog_flush()
{
	time_t timep;
	struct tm *p;
	int i;

	time(&timep);
	p=localtime(&timep);

	for(i = 0; i< packet_count; i++){
	  memset(netlog_buf, '\0', MAX_BUF_SIZE);

	  sprintf(netlog_buf, "%d, %s, %d-%d-%d %02d:%02d:%02d %s\n", log_packet[i].type, log_packet[i].msg, \
		  (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,	\
		  p->tm_hour, p->tm_min, p->tm_sec, wday[p->tm_wday]);
	  fwrite(netlog_buf, strlen(netlog_buf), 1, netlog_fp);
	}

	fflush(netlog_fp);
}
void *test(void *arg)  
{  
  int i, len, tmp_arg = *(int*)arg;
  int sfd = glb_fd;

  while (1) {
  printf("thread %u is waiting for work---------\n", pthread_self());
    pthread_mutex_lock(&(pool->pool_lock));
    len = recvfrom(sfd, &log_packet[ packet_count++ ], sizeof(struct netlog_packet), 0, \
		   (struct sockaddr *) &netlog_addr, &addrlen);
	
    printf("thread %u is working on job %d, the info is: %s, type=%d %d\n",pthread_self(), \
	   tmp_arg, log_packet[ packet_count -1 ].msg, log_packet[ packet_count -1].type, packet_count);

    if(packet_count > 10){
      netlog_flush();
      packet_count = 0;
    }

    pthread_mutex_unlock(&(pool->pool_lock));
  }
      
  return NULL;  
}

#if 0
unsigned short int get_thread()
{
  int i;
  for(i = 0; i < pool->thread_num; i++){
    if(pool->thread_info.used[ pool->thread_info.curr_thread[ i ] ] == 1 && i != (pool->thread_num - 1) )
      continue;

    if( pool->thread_info.used[ pool->thread_info.curr_thread[ i ] ] != 1){
      pool_add_job(test, &i);
      return i;
    }
    else{
      i = pool->thread_num - 1;
      pool_add_job(test, &i);
      return i;
    }
  }
}
#endif

int main(int argc, char *argv[])
{
    FILE *fp;
    u32 sfd, i;

    netlog_parse_cmd(&argc, argv);

    if (!netlog_check_instance()) {
	DEBUG_MSG("There's one netlog instance is running at least!\n");
	exit(1);
    }

    netlog_init_signals();

    if (DAEMON_ON)
	netlog_init_daemon();

    netlog_write_pid();

    printf("%s %d\n", log_conf.netlog_path, log_conf.netlog_port);

    pool_init(log_conf.threads_num);

    addrlen = sizeof(struct sockaddr_in);
    glb_fd = sfd = open_netlog_socket(log_conf.netlog_port, &netlog_addr);

    memset(log_packet, 0x00, MAX_PACKETS_NUM * sizeof(struct netlog_packet));

    for(i = 0; i < log_conf.threads_num; i++)
      pool_add_job(test, &i);

    while(1)
      sleep(10);

    return 0;
}
