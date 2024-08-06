CFLAGS=-std=c2x -g -Wall 

OBJECTS=lib_cards.o lib_token_ring.o 

dane-se:main.c $(OBJECTS)
	gcc $(OBJECTS) main.c -o dane-se $(CFLAGS) 

lib_cards.o:lib_cards.c 
	gcc -c lib_cards.c -o lib_cards.o $(CFLAGS)

lib_token_ring.o:lib_token_ring.c
	gcc -c lib_token_ring.c -o lib_token_ring.o $(CFLAGS)

clean:	
	rm -f dane-se $(OBJECTS)
