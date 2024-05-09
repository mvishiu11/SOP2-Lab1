CC=gcc
CFLAGS=-std=gnu99 -Wall
LDFLAGS=-pthread -lrt 

l4_server: l4_server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

l4_client: l4_client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

l4_client_tcp: l4_client_tcp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)