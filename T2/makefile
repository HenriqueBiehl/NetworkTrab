CFLAGS=-std=c2x -g -Wall 

OBJECTS=lib_cards.o

ring_token:ring_token.c $(OBJECTS)
	gcc $(OBJECTS) ring_token.c -o ring_token $(CFLAGS) 

lib_cards.o:lib_cards.c 
	gcc -c lib_cards.c -o lib_cards.o $(CFLAGS)

clean:	
	rm -f ring_token $(OBJECTS)
