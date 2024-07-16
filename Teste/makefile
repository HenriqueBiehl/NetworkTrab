CFLAGS = -std=c2x -Wall -g

OBJECTS = client server 

client: network.c client.c server
	gcc network.c client.c -o client $(CFLAGS)

server: network.c server.c 
	gcc network.c server.c -o server $(CFLAGS)

clean:
	rm -rf $(OBJECTS)
