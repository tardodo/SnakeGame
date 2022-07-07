all: server client

server: serverTCP.c 
	gcc -o server serverTCP.c playerSnake.c -lnsl -lpthread -Wall

client: clientTCP.c
	gcc -o client clientTCP.c -lnsl -lncurses
