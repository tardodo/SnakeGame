#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>


#include <sys/types.h>

//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>

// #define PORT    5001
#define MAXMSG  512

// Socket path
char *socket_path = "\0hidden";
char *values[FD_SETSIZE];
bool allowSend[FD_SETSIZE];
fd_set active_fd_set;
int fd;
bool startPlayer[FD_SETSIZE];

void initializePlayer(){

}

int read_from_client(int filedes) {

    char buffer[MAXMSG];
    int nbytes;
    memset(buffer, 0, MAXMSG);

    nbytes = read(filedes, buffer, MAXMSG);

    if (nbytes < 0) {
        // Read error
        perror("read");
        close(filedes);
        exit(EXIT_FAILURE);
    } 
    else if (nbytes == 0)
        // End-of-file.
        return -1;
    else {
        // Data read.
        fprintf(stderr, "Server: got message: `%s`\n", buffer);

        if(strcmp(buffer, "snake\n") == 0){

            //FD_SET(filedes, &active_fd_set);
            buffer[5] = '\0';
            initializePlayer();
            allowSend[filedes] = true;
            startPlayer[filedes] = true;
            //return 0;
        }

        allowSend[filedes] = true;

        values[filedes] = (char *)malloc(MAXMSG * sizeof(char));
        strcpy(values[filedes], buffer);


        //memcpy(values[filedes], buffer, MAXMSG);

        // if ((nbytes = write(filedes, "I got your message", 18)) < 0) {
        //     close(filedes);
        //     perror("ERROR writing to socket");
        //     exit(EXIT_FAILURE);
        // }
        return 0;
    }
}

int write_to_client(int filedes) {

    // char buffer[MAXMSG];
    int nbytes;
    // memset(buffer, 0, MAXMSG);
    char buffer[MAXMSG];
    memset(buffer, 0, MAXMSG);
    int len = 0;
    char temp[MAXMSG];

    for (int i = 0; i < FD_SETSIZE; ++i){
        if (FD_ISSET (i, &active_fd_set) && i != fd && startPlayer[i]) {
            //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
            sprintf(temp, "%d, ", i);
            strcat(buffer, values[i]);
            strcat(buffer, temp);
        }
    }

    


    if ((nbytes = write(filedes, buffer, strlen(buffer))) < 0) {
        close(filedes);
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }

    //allowSend[filedes] = false;


    // nbytes = read(filedes, buffer, MAXMSG);

    // if (nbytes < 0) {
    //     // Read error
    //     perror("read");
    //     close(filedes);
    //     exit(EXIT_FAILURE);
    // } 
    // else if (nbytes == 0)
    //     // End-of-file.
    //     return -1;
    // else {
    //     // Data read.
    //     fprintf(stderr, "Server: got message: `%s`\n", buffer);

    //     // if ((nbytes = write(filedes, "I got your message", 18)) < 0) {
    //     //     close(filedes);
    //     //     perror("ERROR writing to socket");
    //     //     exit(EXIT_FAILURE);
    //     // }
        return 0;
    // }
}

int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_un addr;
	char buf[100];
	int cl,rc;
    fd_set read_fd_set, write_fd_set;
    int i, size;
    bool noReads = false;

	// If path is passed in as argument, use it
	if (argc > 1) 
		socket_path=argv[1];

	// Create socket (note AF_UNIX)
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

    // Allow address reuse
    int opts = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

    // Get current options
    if ((opts = fcntl(fd, F_GETFL)) < 0) {
        perror("Error getting socket options\n");
        close(fd);
        exit(1);
    }

    // Set socket to non-blocking
    opts = (opts | O_NONBLOCK);
    if (fcntl(fd, F_SETFL, opts) < 0) {
        {
            perror("Error setting socket to non-blocking");
            close(fd);
            exit(1);
        }
    }


    //------------------------------------------------------------

	// Set address structure
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;

	// Copy path to sun_path in address structure
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
		unlink(socket_path);
	}

	// Bind to address
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	// Listen
	if (listen(fd, 1) == -1) {
		perror("listen error");
		exit(-1);
	}

    //------------------------------------------------------------

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (fd, &active_fd_set);

    for(int j = 0; j < FD_SETSIZE; ++j){
        allowSend[j] = false;
        startPlayer[j] = false;
    }

	// Execute forever
	while (1) {
        noReads = true;

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        write_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, NULL) < 0) {
            perror("select");
            close(fd);
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == fd) {
                    /* Connection request on original socket. */
                    int new;
                    //size = sizeof(clientname);

                    new = accept(fd, NULL, NULL);
                    if (new < 0) {
                        close(fd);
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }

                    // fprintf(stderr,
                    //         "Server: connect from host %s, port %d.\n",
                    //         inet_ntoa(clientname.sin_addr),
                    //         ntohs(clientname.sin_port));
                    fprintf(stderr, "Server: Connected New Client\n");

                    FD_SET (new, &active_fd_set);
                    // allowSend[new] = true;
                    
                } else {
                    /* Data arriving on an already-connected socket. */
                    if (read_from_client(i) < 0) {
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }

                noReads = false;
            }
            // }else if (FD_ISSET (i, &write_fd_set) && allowSend[i]){
            //     if(i != fd){
            //         if(write_to_client(i) < 0){
            //             close(i);
            //             FD_CLR (i, &write_fd_set);
            //         }
            //     }
            // }
        }

		if(noReads){

            sleep(3);
        for (i = 0; i < FD_SETSIZE; ++i){
            if (FD_ISSET (i, &write_fd_set) &&  !FD_ISSET(i, &read_fd_set) && startPlayer[i]){
                if(i != fd){
                    if(write_to_client(i) < 0){
                        close(i);
                        FD_CLR (i, &write_fd_set);
                    }
                }
            }
        }

            for(int j = 0; j < FD_SETSIZE; ++j){
                if(allowSend[j]) allowSend[j] = false;
            }
        }

        /* ------------------------------------------------------------------
		// Accept new client
		if ((cl = accept(fd, NULL, NULL)) == -1) {
		  perror("accept error");
		  continue;
		}

		// Read from client
		while ((rc = read(cl,buf,sizeof(buf))) > 0) {
		   printf("read %u bytes: %.*s\n", rc, rc, buf);
		}
		
		if (rc == -1) {
		  perror("read");
		  exit(-1);
		}
		else if (rc == 0) {
		  printf("EOF\n");
		  close(cl);
		}
        */
	}


	return 0;
}