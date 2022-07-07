

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
#include "playerSnake.h"

#define BACKLOG 50
#define MAXMSG 2048
#define ROWS 20
#define COLS 50

enum direction{up, down, left, right};

char *socket_path = "\0hidden";
char *vals[FD_SETSIZE];
bool startPlayer[FD_SETSIZE];
char fruit[100][7];
bool gameStart;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;

const int width = 20, height = 20;
const int rows  = width, cols = width*2.5;
char arenaMap[ROWS][COLS];

typedef struct pthread_arg_t {
    int new_socket_fd;
    //struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

/* Thread routine to serve connection to client. */
void *clientThread(void *arg);

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
            
            pthread_mutex_lock(&mutex4);
            if(!gameStart) gameStart = true;
            pthread_mutex_unlock(&mutex4);

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
    int length = 0;
    char temp[MAXMSG];

    memset(buffer, 0, MAXMSG);

    for (int i = 0; i < FD_SETSIZE; ++i){
        pthread_mutex_lock(&mutex2);
        if (startPlayer[i]) {
            //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
            //sprintf(temp, "%d, ", i);

            pthread_mutex_lock(&mutex1);
            strcat(buffer, vals[i]);
            pthread_mutex_unlock(&mutex1);
            strcat(buffer, "|");
            // strcat(buffer, temp);
        }
        pthread_mutex_unlock(&mutex2);
    }

    strcat(buffer, "$");

    for (int i = 0; i < 100; ++i){
        pthread_mutex_lock(&mutex3);
        length = strlen(fruit[i]);
        pthread_mutex_unlock(&mutex3);

        if(length > 0){
            //memset(buffer, 0, MAXMSG);

            pthread_mutex_lock(&mutex3);
            strcat(buffer, fruit[i]);
            pthread_mutex_unlock(&mutex3);
            strcat(buffer, ",");
        }
    }


    // memset(buffer, 0, MAXMSG);
    // strcat(buffer, values);
    // strcat(buffer, temp);

      
      //sleep(1);

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

void *spawnFruit(void* arg){

    bool valid = false;
    int x,y;
    char buffer[MAXMSG];
    char *temp;
    int snakeX;
    int snakeY;
    int fruitX,fruitY;
    int delay;
    int keepLooping;
    int numOfFruit = 0;
    

    sleep(10);
    // pthread_mutex_lock(&mutex4);
    // keepLooping = gameStart;
    // pthread_mutex_unlock(&mutex4);
    keepLooping = true;
    
    do{

        numOfFruit = 0;
        valid = false;

        while(!valid){
            numOfFruit = 0;
            x = random()%(cols -2) + 1;
            y = random()%(rows - 2) + 1;
            // y = 1;
            valid = true;
            
            //---------------------------------------------------------
            pthread_mutex_lock(&mapMutex);
            if(arenaMap[y][x] != '#' &&  arenaMap[y][x] != '*')
                arenaMap[y][x] = '*';
            else valid = false;
            pthread_mutex_unlock(&mapMutex);
            //----------------------------------------------------------

            // for (int i = 0; i < FD_SETSIZE; ++i){
            //     pthread_mutex_lock(&mutex2);
            //     if (startPlayer[i]) {
            //     //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
            //     //sprintf(temp, "%d, ", i);
            //         memset(buffer, 0, MAXMSG);
    
            //         pthread_mutex_lock(&mutex1);
            //         strcpy(buffer, vals[i]);
            //         pthread_mutex_unlock(&mutex1);

            //         if(strcmp(buffer, "snake") != 0 && strchr(buffer, '-') == NULL){
            //         temp = strtok(buffer, ",");
            //         snakeX = atoi(temp);
            //         temp = strtok(NULL, ",");
            //         snakeY = atoi(temp);
            //         temp = strtok(NULL, ",");
            //         if(temp != NULL){
            //             snakeX = atoi(temp);
                        
            //             temp = strtok(NULL, ",");
            //             snakeY = atoi(temp);
            //         }

            //         if(x == snakeX && y == snakeY){
            //             valid = false;
            //             pthread_mutex_unlock(&mutex2);
            //             break;
            //         }
            //         }

            //     }
            //     pthread_mutex_unlock(&mutex2);

            //     int length;
            //     if(i < 100){
                    
            //         pthread_mutex_lock(&mutex3);
            //         length = strlen(fruit[i]);
            //         pthread_mutex_unlock(&mutex3);

            //         if(length > 0){
            //             ++numOfFruit;
            //             memset(buffer, 0, MAXMSG);

            //             pthread_mutex_lock(&mutex3);
            //             strcpy(buffer, fruit[i]);
            //             pthread_mutex_unlock(&mutex3);

            //             temp = strtok(buffer, ",");
            //             fruitX = atoi(temp);
            //             temp = strtok(NULL, ",");
            //             fruitY = atoi(temp);

            //             if(x == fruitX && y == fruitY){
            //                 valid = false;
            //                 break;
            //             }
            //         }
            //     }
            // }
        }

        for(int i = 0; i < 100; ++i){
            pthread_mutex_lock(&mutex3);
            if(strlen(fruit[i]) == 0){
                sprintf(fruit[i], "%d,%d", x, y);
                pthread_mutex_unlock(&mutex3);
                printf("Generated fruit at %d, %d\n", x, y);
                break;
            }
            pthread_mutex_unlock(&mutex3);
        }

        delay = random()%(15 + numOfFruit);

        sleep(delay);

        pthread_mutex_lock(&mutex4);
        keepLooping = gameStart;
        pthread_mutex_unlock(&mutex4);

    }while(keepLooping);

    for(int i = 0; i < 100; ++i){
        pthread_mutex_lock(&mutex3);
        if(strlen(fruit[i]) > 0){
            strcpy(fruit[i], "\0");
                //pthread_mutex_unlock(&mutex3);
                //printf("Generated fruit at %d, %d\n", x, y);
                //break;
        }
        pthread_mutex_unlock(&mutex3);
    }

    pthread_exit(NULL);



}

int main(int argc, char *argv[]) {
    int port, socket_fd, new_socket_fd;
    struct sockaddr_un addr;
    pthread_attr_t pthread_attr;
    pthread_arg_t pthread_arg[BACKLOG];
    pthread_t pthread[BACKLOG];
    socklen_t client_address_len;
    int numOfThreads = 0;
    bool spawnFruitThread = false;
    bool newGame = false;

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
    gameStart = false;

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

        pthread_mutex_lock(&mutex4);
        newGame = !gameStart;
        pthread_mutex_unlock(&mutex4);

        if(newGame){
            printf("\n\nNew game !\n\n");
            if(numOfThreads > 0){

                for(int i = 0; i <numOfThreads; i++){
                    pthread_join(pthread[i], NULL);
                }

            }

            for(int i = 0; i< ROWS; ++i){
                memset(arenaMap[i], ' ', COLS);
            }

            numOfThreads = 0;
            if (pthread_create(&pthread[numOfThreads], NULL, spawnFruit, NULL) != 0) {
                perror("pthread_create_fruit");
                // free(pthread_arg);
                exit(EXIT_FAILURE);
            }

            printf("Spawned Fruit Thread\n");
            ++numOfThreads;
        }

        // if(numOfThreads == 0){
        //     if (pthread_create(&pthread[numOfThreads], NULL, spawnFruit, NULL) != 0) {
        //         perror("pthread_create_fruit");
        //         // free(pthread_arg);
        //         exit(EXIT_FAILURE);
        //     }
        //     ++numOfThreads;
        // }

        /* Initialise pthread argument. */
        // pthread_arg->new_socket_fd = new_socket_fd;
        pthread_arg[numOfThreads].new_socket_fd = new_socket_fd;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */

        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread[numOfThreads], NULL, clientThread, &pthread_arg[numOfThreads]) != 0) {
            perror("pthread_create");
            // free(pthread_arg);
            continue;
        }

        ++numOfThreads;

        
        if (numOfThreads >= BACKLOG) {
            // Update i
            numOfThreads = 1;
 
            while (numOfThreads < BACKLOG) {
                // Suspend execution of
                // the calling thread
                // until the target
                // thread terminates
                pthread_join(pthread[numOfThreads++],
                             NULL);
                // pthread_join(readerthreads[i++],
                //              NULL);
            }
 
            // Update i
            numOfThreads = 1;
        }
    }

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signal_handler(), meaning socket_fd would need to be a global variable.
     */
    return 0;
}

void *clientThread(void *arg) {
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
    bool wallHit;

    // start snake
    if (read_from_client(new_socket_fd) < 0) {
      close(new_socket_fd);
      pthread_exit(NULL);
    }

    memset(buffer, 0, MAXMSG);
    sprintf(buffer, "%d,%d,", rows, cols);
    // strcat(buffer, values);
    // strcat(buffer, temp);

    //send rows and cols
    if ((nbytes = write(new_socket_fd, buffer, strlen(buffer))) < 0) {
        close(new_socket_fd);
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }

    printf("Sent rows and cols\n");

    bool valid = false;
    int x,y;
    int length;
    int currDir;
    Snake *snake;
    Snake *head;
    bool justSpawned = true;

    //sleep(1);
    memset(buffer, 0, MAXMSG);
    
    while(!valid){
        x = random()%(cols -2) + 1;
        y = random()%(rows - 2) + 1;
        // y = 1;
        valid = true;

        // ----------------------------------------------------------
        pthread_mutex_lock(&mapMutex);
        if(arenaMap[y][x] != '#' && arenaMap[y][x] != '*'){
            
            if((y + 5) < (rows) && arenaMap[y + 1][x] != '#' && arenaMap[y + 2][x] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = down;
                sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x, y+1, x, y+2);

            }else if((y - 5) > 0 && arenaMap[y - 1][x] != '#' && arenaMap[y - 2][x] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y-1][x] = '#';
                arenaMap[y-2][x] = '#';
                currDir = up;
                sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x, y-1, x, y-2);

            }else if((x + 5) < (cols) && arenaMap[y][x+1] != '#' && arenaMap[y][x+2] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = right;
                sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x+1, y, x+2, y);

            }else if((x - 5) > 0 && arenaMap[y][x-1] != '#' && arenaMap[y][x-2] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = left;
                sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x-1, y, x-2, y);

            }else valid = false;

            //arenaMap[y][x] = '#';

        }else valid = false;

        pthread_mutex_unlock(&mapMutex);
        
        //-------------------------------------------------------------

        // for (int i = 0; i < FD_SETSIZE; ++i){
        //     pthread_mutex_lock(&mutex2);
        //     if (startPlayer[i] && i != new_socket_fd) {
        //     //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
        //     //sprintf(temp, "%d, ", i);
        //         memset(buffer, 0, MAXMSG);
    
        //         pthread_mutex_lock(&mutex1);
        //         strcpy(buffer, vals[i]);
        //         pthread_mutex_unlock(&mutex1);

        //         char *temp;
        //         int enemyX;
        //         int enemyY;
               
        //         if(strcmp(buffer, "snake") != 0 && strchr(buffer, '-') == NULL){
        //         temp = strtok(buffer, ",");
        //         enemyX = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         enemyY = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         if(temp != NULL){
        //             enemyX = atoi(temp);
        //             temp = strtok(NULL, ",");
        //             enemyY = atoi(temp);
        //         }

        //         if(x == enemyX && y == enemyY){
        //             valid = false;
        //             pthread_mutex_unlock(&mutex2);
        //             break;
        //         }
        //         }

        //     }
        //     pthread_mutex_unlock(&mutex2);

        //     if(i < 100){
                
        //         pthread_mutex_lock(&mutex3);
        //         length = strlen(fruit[i]);
        //         pthread_mutex_unlock(&mutex3);

        //         if(strlen(fruit[i]) > 0){
        //             memset(buffer, 0, MAXMSG);

        //             pthread_mutex_lock(&mutex3);
        //             strcpy(buffer, fruit[i]);
        //             pthread_mutex_unlock(&mutex3);

        //             char *temp;
        //             int fruitX;
        //             int fruitY;

        //             temp = strtok(buffer, ",");
        //             fruitX = atoi(temp);
        //             temp = strtok(NULL, ",");
        //             fruitY = atoi(temp);

        //             if(x == fruitX && y == fruitY){
        //                 valid = false;
        //                 break;
        //             }
        //         }
                
        //     }
        // }
    }

    snake = initializeSnake(NULL, x, y, currDir);
    head = getTail(NULL, snake);

    x = head->x;
    y = head->y;

    printf("Computed x and y\n");

    

    pthread_mutex_lock(&mutex1);
    vals[new_socket_fd] = (char *)malloc(MAXMSG * sizeof(char));
    strcpy(vals[new_socket_fd], buffer);
    pthread_mutex_unlock(&mutex1);

    printf("Allocated x and y\n");

    //--------------------------------------------------------------------------
    memset(buffer, 0, MAXMSG);
    sprintf(buffer, "%d,%d,%d,", x, y, currDir);

    // strcat(buffer, ",");
    //memset(buffer, 0, MAXMSG);
    pthread_mutex_lock(&mapMutex);
    strcat(buffer, arenaMap[0]);
    pthread_mutex_unlock(&mapMutex);
    
   
    //------------------------------------------------------------------------

    if ((nbytes = write(new_socket_fd, buffer, strlen(buffer))) < 0) {
        close(new_socket_fd);
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }

    printf("Sent x and y\n");
    

    // if (write_to_client(new_socket_fd) < 0) {
    //   close(new_socket_fd);
    //   pthread_exit(NULL);
    // }

    // sprintf(temp, "%d, ", new_socket_fd);
    //         strcat(buffer, values[i]);
    //         strcat(buffer, temp);
    //     }
    // }

    // if (read_from_client(new_socket_fd) < 0) {
    //   close(new_socket_fd);
    //   pthread_exit(NULL);
    // }


    // if ((nbytes = read(new_socket_fd, buffer, strlen(buffer))) < 0) {
    //     close(new_socket_fd);
    //     perror("ERROR read");
    //     exit(EXIT_FAILURE);
    // }
    // printf("%s", buffer);


    // memset(buffer, 0, 256);

    char *temp2;
    char *rest;
    int newx = x, newy = y;
    int gameOver = false;
    //int length = 0;
    bool eaten = false;
    wallHit = false;

    // strcpy(buffer, "I got your message");
    while(!gameOver){

        // Assign tail to x and y
        x = snake->x;
        y = snake->y;
        
        
        // if(!justSpawned){

        memset(buffer, 0, MAXMSG);
        temp2 = NULL;
        rest = NULL;
        
        eaten = false;

        if ((nbytes = read(new_socket_fd, &currDir, sizeof(int))) < 0) {
            close(new_socket_fd);
            perror("ERROR read");
            exit(EXIT_FAILURE);
        }

        if(nbytes == 0){

            gameOver = true;
            // break;
        }

        // pthread_mutex_lock(&mutex1);
        // //vals[new_socket_fd] = (char *)malloc(MAXMSG * sizeof(char));
        // strcpy(buffer, vals[new_socket_fd]);
        // pthread_mutex_unlock(&mutex1);

        //temp2 = strtok(buffer, ",");
        rest = buffer;
        temp2 = strtok_r(rest, ",", &rest);

        // x= atoi(temp2);
        // // temp2 = strtok(NULL, ",");
        // temp2 = strtok_r(rest, ",", &rest);
        // y = atoi(temp2);
        // // temp2 = strtok(NULL, ",");
        // temp2 = strtok_r(rest, ",", &rest);
        // if(temp2 != NULL){
        //     newx = atoi(temp2);
        //     // temp2 = strtok(NULL, ",");
        //     temp2 = strtok_r(rest, ",", &rest);
        //     newy = atoi(temp2);

        //     x = newx;
        //     y = newy;

        //     // sprintf(buffer, "%d,%d,%d,%d", newx, newy, newx, newy + 1);
        // }else{

        //     newx = x;
        //     newy = y;
        //     // sprintf(buffer, "%d,%d,%d,%d", x, y, x, y + 1);
        // }

        switch(currDir){
            case up: 
                if ( newy > 1){
                    //mvwaddch(arena, y, x, ' ');
		            --newy;
                    
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
                break;

            case down:
                if ( newy < rows - 2){
                    //mvwaddch(arena, y, x, ' ');
		            ++newy;
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
	        break;

            case left:
                if ( newx > 1 ){
                    //mvwaddch(arena, y, x, ' ');
		            --newx;
                }
                else{
                    gameOver = true; 
                    wallHit = true;
                }
	        break;

            case right:
                if ( newx < cols - 2){
                    //mvwaddch(arena, y, x, ' ');
		            ++newx;
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
	        break;

            default: break;
        }

        //check collision

        //---------------------------------------------------
        pthread_mutex_lock(&mapMutex);
        if(arenaMap[newy][newx] == '#'){
            gameOver = true;
        }else if(arenaMap[newy][newx] == '*'){
            eaten = true;
            // pthread_mutex_lock(&mutex3);
            // strcpy(fruit[i], "\0");
            // pthread_mutex_unlock(&mutex3);
        }
        pthread_mutex_unlock(&mapMutex);
        //---------------------------------------------------
        
        // for (int i = 0; i < FD_SETSIZE; ++i){
        //     pthread_mutex_lock(&mutex2);
        //     if (startPlayer[i] && i != new_socket_fd) {
        //     //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
        //     //sprintf(temp, "%d, ", i);
        //         memset(buffer, 0, MAXMSG);
    
        //         pthread_mutex_lock(&mutex1);
        //         strcpy(buffer, vals[i]);
        //         pthread_mutex_unlock(&mutex1);

        //         char *temp;
        //         int enemyX;
        //         int enemyY;
               
        //         if(strcmp(buffer, "snake") != 0 && strchr(buffer, '-') == NULL){
        //         temp = strtok(buffer, ",");
        //         enemyX = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         enemyY = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         if(temp != NULL){
        //             enemyX = atoi(temp);
        //             temp = strtok(NULL, ",");
        //             enemyY = atoi(temp);
        //         }

        //         if(newx == enemyX && newy == enemyY){
        //             gameOver = true;
        //             pthread_mutex_unlock(&mutex2);
        //             break;
        //         }
        //         }

        //     }
        //     pthread_mutex_unlock(&mutex2);
        // }

        if(!gameOver && eaten){
            for (int i = 0; i < 100; ++i){
                pthread_mutex_lock(&mutex3);
                length = strlen(fruit[i]);
                pthread_mutex_unlock(&mutex3);

                if(length > 0){
                    memset(buffer, 0, MAXMSG);

                    pthread_mutex_lock(&mutex3);
                    strcpy(buffer, fruit[i]);
                    pthread_mutex_unlock(&mutex3);

                    char *temp;
                    int fruitX;
                    int fruitY;

                    temp = strtok(buffer, ",");
                    fruitX = atoi(temp);
                    temp = strtok(NULL, ",");
                    fruitY = atoi(temp);

                    if(newx == fruitX && newy == fruitY){
                        //eaten = true;
                        pthread_mutex_lock(&mutex3);
                        strcpy(fruit[i], "\0");
                        pthread_mutex_unlock(&mutex3);
                        break;
                    }
                }
            }

        }

        if(gameOver){
            char part[10];
            memset(buffer, 0, MAXMSG);
            strcat(buffer, "-");

            while(snake != NULL){
                sprintf(part, "%d,%d,", snake->x, snake->y);
                strcat(buffer, part);
                pthread_mutex_lock(&mapMutex);
                arenaMap[snake->y][snake->x] = ' ';
                pthread_mutex_unlock(&mapMutex);
                snake = snake->nextPart;
            }
            // sprintf(buffer, "-%d,%d", x, y);

            // pthread_mutex_lock(&mapMutex);
            // arenaMap[y][x] = ' ';
            // pthread_mutex_unlock(&mapMutex);

        // }else if(eaten){
        //     sprintf(buffer, "+%d,%d", newx, newy);
        }else{
            
            
            sprintf(buffer, "%d,%d,%d,%d", x, y, newx, newy);

            pthread_mutex_lock(&mapMutex);
            arenaMap[newy][newx] = '#';
            arenaMap[y][x] = ' ';
            pthread_mutex_unlock(&mapMutex);

            snake = moveSnake(NULL, newx, newy, &snake);
        }
        

        // sprintf(buffer, "%d,%d", x, ++y);

        pthread_mutex_lock(&mutex1);
        //vals[new_socket_fd] = (char *)malloc(MAXMSG * sizeof(char));
        strcpy(vals[new_socket_fd], buffer);
        pthread_mutex_unlock(&mutex1);

    
        // End of JustSpawned --------------------------------------
        // }
        
        sleep(0.5);

        if(!gameOver || wallHit){

            if (write_to_client(new_socket_fd) < 0) {
                close(new_socket_fd);
                pthread_exit(NULL);

            }
        }

        if(justSpawned) justSpawned = false;

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

    // x = newx;
    // y = newy;
    
    }

    sleep(2);

    pthread_mutex_lock(&mutex2);
    startPlayer[new_socket_fd] = false;
    pthread_mutex_lock(&mutex4);
    gameStart = false;
    for (int i = 0; i < FD_SETSIZE; ++i){
        if(startPlayer[i]){
            gameStart = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex4);
    pthread_mutex_unlock(&mutex2);

    
    printf("Exiting Thread %d\n", new_socket_fd);
    close(new_socket_fd);
    pthread_exit(NULL);
    //return NULL;
}

void signal_handler(int signal_number) {
    /* TODO: Put exit cleanup code here. */
    exit(0);
}
