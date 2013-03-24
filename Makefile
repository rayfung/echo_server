all: server tcp_client udp_client
server: server.c
	gcc -O2 -o $@ $^
tcp_client: client.c
	gcc -O2 -o $@ $^
udp_client: client.c
	gcc -DUSE_UDP -O2 -o $@ $^
clean:
	rm -f server tcp_client udp_client
