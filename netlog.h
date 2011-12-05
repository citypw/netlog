/**
 * Declarations of netlog stuffs including struct, variable and constatn.
 * Copyright(C) 2011, <Shawn the R0ck, citypw@gmail.com>
 * 
 * The netlog is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * Version 3(GPLv3) as published by the Free Software Foundation.
 **/

#ifndef _NET_LOG_H
#define _NET_LOG_H
/* Using these 2 macros instead of traditional printf */
#define DEBUG_ERR(format, args...) fprintf(stderr, format, ##args)

#define DEBUG_MSG(format, args...) do{ \
	DEBUG_ERR("+--------------------------------------------------------------- \
\n| File: %s, Function: %s, Line: %d\n| Description: ",\
		  __FILE__, __FUNCTION__, __LINE__);			\
	DEBUG_ERR(format, ##args); \
	DEBUG_ERR("+---------------------------------------------------------------\n"); \
}while(0);

#define PID_FILE  "/tmp/netlog.pid"

/* short name could saving time */
typedef unsigned short int u16;
typedef unsigned int u32;
typedef short int s16;
typedef int s32;

/* 0: No --- 1: Yes */
static unsigned short DAEMON_ON = 0;

typedef enum _getopt_arg_type
{
    LONGOPT_ARG_NONE = 0,
    LONGOPT_ARG_REQUIRED,
    LONGOPT_ARG_OPTIONAL
} getopt_arg_type;

/* command line options for getopt */
char *valid_opts = "DHVC:I:";

/* declarations of netlog functions */
void netlog_parse_cmd(int*, char **);
void netlog_init_daemon();
int netlog_check_instance();
void netlog_write_pid();
u32 get_tick_count();
void netlog_init_signals();
struct netlog_conf netlog_read_ini(FILE *);
u16 netlog_flush_init();
void netlog_flush();


/* definition of log type */
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_DEBUG 3

#define MAX_MSG_SIZE 256
#define MAX_BUF_SIZE 512
#define MAX_PACKETS_NUM 16000

/* the parameters of netlog in config file */
struct netlog_conf {
  char netlog_path[MAX_MSG_SIZE];
  u16 netlog_port;
  u16 threads_num;
};

struct netlog_packet{
  //  struct netlog_packet *next; //for next version?
  u16 type;
  //  u16 size;
  char msg[MAX_MSG_SIZE];
}__attribute__ ((packed));

#endif
