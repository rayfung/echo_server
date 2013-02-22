all: server client
server: server.c
	gcc -O2 -o $@ $^
client: client.c
	gcc -O2 -o $@ $^
clean:
	rm -f server client
