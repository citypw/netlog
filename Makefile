CC=gcc
CFLAGS= -g
LIBS = -lpthread

NETLOG_SERVER = netlogd
NETLOG_CLIENT = netlog_client

APP = SERVER CLIENT

all:$(APP)
	@echo "done"

SERVER: $(NETLOG_SERVER)

netlogd: netlog.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

CLIENT: $(NETLOG_CLIENT)

netlog_client: netlog_client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(NETLOG_SERVER) $(NETLOG_CLIENT)