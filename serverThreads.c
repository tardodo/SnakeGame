

/*
 * server.c
 * Version 20161003
 * Written by Harry Wong (RedAndBlueEraser)
 */

#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#include <fcntl.h>

#include <sys/un.h>

#include <stdbool.h>
#include <errno.h>


#include <sys/types.h>

#define BACKLOG 10
#define MAXMSG 526

char *socket_path = "\0hidden";
char *vals[FD_SETSIZE];
bool startPlayer[FD_SETSIZE];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
const int width = 20, height = 20;
const int rows  = width, cols = width*2.5;

typedef struct pthread_arg_t {
    int new_socket_fd;
    //struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

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
            pthread_mutex_lock(&mutex2);
            startPlayer[filedes] = true;
            pthread_mutex_unlock(&mutex2);

            // allowSend[filedes] = true;
            // startPlayer[filedes] = true;
            //return 0;
        }

        //values = (char *)malloc(MAXMSG * sizeof(char));
        pthread_mutex_lock(&mutex1);
        vals[filedes] = (char *)malloc(MAXMSG * sizeof(char));
        strcpy(vals[filedes], buffer);
        pthread_mutex_unlock(&mutex1);

        // allowSend[filedes] = true;

        // values[filedes] = (char *)malloc(MAXMSG * sizeof(char));
        // strcpy(values[filedes], buffer);


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

    memset(buffer, 0, MAXMSG);

    for (int i = 0; i < FD_SETSIZE; ++i){
        pthread_mutex_lock(&mutex2);
        if (startPlayer[i]) {
            //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
            sprintf(temp, "%d, ", i);

            pthread_mutex_lock(&mutex1);
            strcat(buffer, vals[i]);
            pthread_mutex_unlock(&mutex1);
            
            strcat(buffer, temp);
        }
        pthread_mutex_unlock(&mutex2);
    }
    // memset(buffer, 0, MAXMSG);
    // strcat(buffer, values);
    // strcat(buffer, temp);

      
      sleep(1);

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

int main(int argc, char *argv[]) {
    int port, socket_fd, new_socket_fd;
    struct sockaddr_un addr;
    pthread_attr_t pthread_attr;
    pthread_arg_t pthread_arg[BACKLOG];
    pthread_t pthread[BACKLOG];
    socklen_t client_address_len;
    int i = 0;

    /* Get port from command line arguments or stdin. */
    // port = argc > 1 ? atoi(argv[1]) : 0;
    // if (!port) {
    //     printf("Enter Port: ");
    //     scanf("%d", &port);
    // }

   if ( (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

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
	if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	// Listen
	if (listen(socket_fd, 10) == -1) {
		perror("listen error");
		exit(-1);
	}

    /* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    /* Initialise pthread attribute to create detached threads. */
    // if (pthread_attr_init(&pthread_attr) != 0) {
    //     perror("pthread_attr_init");
    //     exit(1);
    // }
    // if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
    //     perror("pthread_attr_setdetachstate");
    //     exit(1);
    // }

    for(int j = 0; j < FD_SETSIZE; ++j){
        startPlayer[j] = false;
    }

    while (1) {
        /* Create pthread argument for each connection to client. */
        /* TODO: malloc'ing before accepting a connection causes only one small
         * memory when the program exits. It can be safely ignored.
         */
        // pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);
        // if (!pthread_arg) {
        //     perror("malloc");
        //     continue;
        // }

        /* Accept connection to client. */
        //client_address_len = sizeof pthread_arg->client_address;
        new_socket_fd = accept(socket_fd, NULL, NULL);
        if (new_socket_fd < 0) {
          close(socket_fd);
          perror("accept");
          // free(pthread_arg);
          exit(EXIT_FAILURE);
        }else{
          printf("Connected to Client!");
        }

        /* Initialise pthread argument. */
        // pthread_arg->new_socket_fd = new_socket_fd;
        pthread_arg[i].new_socket_fd = new_socket_fd;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */

        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread[i], NULL, pthread_routine, &pthread_arg[i]) != 0) {
            perror("pthread_create");
            // free(pthread_arg);
            continue;
        }

        ++i;

        if (i >= BACKLOG) {
            // Update i
            i = 0;
 
            while (i < BACKLOG) {
                // Suspend execution of
                // the calling thread
                // until the target
                // thread terminates
                pthread_join(pthread[i++],
                             NULL);
                // pthread_join(readerthreads[i++],
                //              NULL);
            }
 
            // Update i
            i = 0;
        }
    }

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signal_handler(), meaning socket_fd would need to be a global variable.
     */
    return 0;
}

void *pthread_routine(void *arg) {
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    //struct sockaddr_in client_address = pthread_arg->client_address;
    /* TODO: Get arguments passed to threads here. See lines 22 and 116. */

    //free(arg);

    printf("Inside Thread %d\n,", new_socket_fd);

    /* TODO: Put client interaction code here. For example, use
     * write(new_socket_fd,,) and read(new_socket_fd,,) to send and receive
     * messages with the client.
     */

    char buffer[MAXMSG];
    char temp[MAXMSG];
    int nbytes;
    memset(buffer, 0, MAXMSG);
    char *values;

    // nbytes = read(new_socket_fd, buffer, MAXMSG);

    // if (nbytes < 0) {
    //     // Read error
    //     perror("read");
    //     close(new_socket_fd);
    //     exit(EXIT_FAILURE);
    // } 
    // else if (nbytes == 0){
    //     // End-of-file.
    //     close(new_socket_fd);
    //     pthread_exit(NULL);
    // }
    // else {
    //     // Data read.
    //     fprintf(stderr, "Server: got message: `%s`\n", buffer);

    //     if(strcmp(buffer, "snake\n") == 0){

    //         //FD_SET(filedes, &active_fd_set);
    //         buffer[5] = '\0';
    //         initializePlayer();
    //         pthread_mutex_lock(&mutex2);
    //         startPlayer[new_socket_fd] = true;
    //         pthread_mutex_unlock(&mutex2);

    //         // allowSend[filedes] = true;
    //         // startPlayer[filedes] = true;
    //         //return 0;
    //     }

    //     //values = (char *)malloc(MAXMSG * sizeof(char));
    //     pthread_mutex_lock(&mutex1);
    //     vals[new_socket_fd] = (char *)malloc(MAXMSG * sizeof(char));
    //     strcpy(vals[new_socket_fd], buffer);
    //     pthread_mutex_unlock(&mutex1);
    // }

    if (read_from_client(new_socket_fd) < 0) {
      close(new_socket_fd);
      pthread_exit(NULL);
    }

    memset(buffer, 0, MAXMSG);
    sprintf(temp, "%d, ", new_socket_fd);
    // strcat(buffer, values);
    // strcat(buffer, temp);

    if ((nbytes = write(new_socket_fd, buffer, strlen(buffer))) < 0) {
        close(new_socket_fd);
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }


    if (write_to_client(new_socket_fd) < 0) {
      close(new_socket_fd);
      pthread_exit(NULL);
    }

    // sprintf(temp, "%d, ", new_socket_fd);
    //         strcat(buffer, values[i]);
    //         strcat(buffer, temp);
    //     }
    // }

    if (read_from_client(new_socket_fd) < 0) {
      close(new_socket_fd);
      pthread_exit(NULL);
    }

    // strcpy(buffer, "I got your message");
    while(1){

      if (write_to_client(new_socket_fd) < 0) {
      close(new_socket_fd);
      pthread_exit(NULL);
    }

      // sleep(1);

    // memset(buffer, 0, MAXMSG);

    // for (int i = 0; i < FD_SETSIZE; ++i){
    //     pthread_mutex_lock(&mutex2);
    //     if (startPlayer[i]) {
    //         //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
    //         sprintf(temp, "%d, ", i);

    //         pthread_mutex_lock(&mutex1);
    //         strcat(buffer, vals[i]);
    //         pthread_mutex_unlock(&mutex1);
            
    //         strcat(buffer, temp);
    //     }
    //     pthread_mutex_unlock(&mutex2);
    // }
    // // memset(buffer, 0, MAXMSG);
    // // strcat(buffer, values);
    // // strcat(buffer, temp);

      
    //   sleep(1);

    // if ((nbytes = write(new_socket_fd, buffer, strlen(buffer))) < 0) {
    //     close(new_socket_fd);
    //     perror("ERROR writing to socket");
    //     exit(EXIT_FAILURE);
    // }

    
    }




    close(new_socket_fd);
    pthread_exit(NULL);
    //return NULL;
}

void signal_handler(int signal_number) {
    /* TODO: Put exit cleanup code here. */
    exit(0);
}
