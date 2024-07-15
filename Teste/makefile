CFLAGS= -std=c2x -Wall -g

OBJECTS=client server 

client: client.c server
	gcc client.c -o client $(CFLAGS)

server: server.c 
	gcc server.c -o server $(CFLAGS)

clean:
	rm -rf $(OBJECTS)