#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>

// Socket path
char *socket_path = "\0hidden";

int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_un addr;
 	char buffer[100];
	int fd, rc, n;

	// If path is passed in as argument, use it
	if (argc > 1) 
		socket_path = argv[1];

	// Create socket (note AF_UNIX)
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	// Set address structure
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	
	// Copy path to sun_path in address structure
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	}

	// Connect 
	if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		perror("connect error");
		exit(-1);
	}


    memset(buffer, 0, 256);
    fgets(buffer, 255, stdin);
    
    //strcpy(buffer, "snake");
    if ((n = write(fd, buffer, strlen(buffer))) < 0)
    {
         perror("ERROR writing to socket");
		    close(fd);
         exit(1);
    }

    memset(buffer, 0, 256);

    if ((n = read(fd, buffer, 255)) < 0)
    {
         perror("ERROR reading from socket");
		      close(fd);
         exit(1);
    }

    // wrefresh(arena);
	
    printf("%s\n", buffer);

    // WINDOW* arena;
    // initscr();
    // refresh();
    // cbreak();
    // noecho();
    // signal (SIGWINCH, NULL);

    // //signal (SIGWINCH, NULL);
    // curs_set(0);

    memset(buffer, 0, 256);
    //fgets(buffer, 255, stdin);
    strcpy(buffer, "Hola there! ");
    
    // sleep(1);

    // // Send message to the server
    if ((n = write(fd, buffer, strlen(buffer))) < 0)
    {
         perror("ERROR writing to socket");
		 close(fd);
         exit(1);
    }

    //---------------------------------------------------------------------------

    while(1){
    // Ask for a message from the user
    //printf("Please enter the message: ");
    // memset(buffer, 0, 256);
    //fgets(buffer, 255, stdin);
    // strcpy(buffer, "Hola there! ");
    //sleep(2);

    // Send message to the server
    // if ((n = write(fd, buffer, strlen(buffer))) < 0)
    // {
    //      perror("ERROR writing to socket");
	// 	 close(fd);
    //      exit(1);
    // }

    //sleep(5);
    sleep(1);

    // Read response from server response
    memset(buffer, 0, 256);

    if ((n = read(fd, buffer, 255)) < 0)
    {
         perror("ERROR reading from socket");
		      close(fd);
         exit(1);
    }

    // wrefresh(arena);
	
    printf("%s\n", buffer);
    }
    close(fd);

    /* ---------------------------------------------------------------------------------

	// Perform read/write
	while((rc = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
	  if (write(fd, buffer, rc) != rc) {
	    if (rc > 0) 
		  fprintf(stderr, "partial write");
	    else {
		  perror("write error");
		  exit(-1);
	    }
	  }
	}
    */

	return 0;
}