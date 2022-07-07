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

#include "playerSnake.h"

#define MAXTHREADS 50
#define MAXMSG 2048
#define ROWS 20
#define COLS 50
#define PORT 5001
#define MAXFRUIT 50

// Direction for movement
enum direction{up, down, left, right};

char *socket_path = "\0hidden";
char *playerCoordinates[FD_SETSIZE];
bool startPlayer[FD_SETSIZE];
char fruitCoordinates[MAXFRUIT][7];
bool gameStart;

// Keep track of number of threads
int numOfThreads = 0;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mapMutex = PTHREAD_MUTEX_INITIALIZER;

// Map to keep track of data internally
char arenaMap[ROWS][COLS];

typedef struct{
    int socketFileDes;
} pthreadArgument;

pthreadArgument pthreadArgs[MAXTHREADS];
pthread_t pthread[MAXTHREADS];

void *acceptLocal(void *arg);
void *spawnFruit(void* arg);
void *clientThread(void *arg);
int readSnakeCommand(int filedes);
int writeAllToClient(int filedes);
void signalHandler(int signal);


int main(int argc, char *argv[]) {
    int port, socket_fd, newSocketFileDes, clilen, local_sock;
    struct sockaddr_in tcp_addr, cli_addr;
    struct sockaddr_un local_addr;
    // pthreadArgument pthreadArgs[MAXTHREADS];
    // pthread_t pthread[MAXTHREADS];
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

   if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

    if ( (local_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

  // Set address structure
	memset(&tcp_addr, 0, sizeof(tcp_addr));
    memset(&local_addr, 0, sizeof(local_addr));

	local_addr.sun_family = AF_UNIX;

	// Copy path to sun_path in address structure
	if (*socket_path == '\0') {
		*local_addr.sun_path = '\0';
		strncpy(local_addr.sun_path+1, socket_path+1, sizeof(local_addr.sun_path)-2);
	} else {
		strncpy(local_addr.sun_path, socket_path, sizeof(local_addr.sun_path)-1);
		unlink(socket_path);
	}

    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(PORT);

	// Bind to address
	if (bind(socket_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	// Listen
	if (listen(socket_fd, 10) == -1) {
		perror("listen error");
		exit(-1);
	}

    if (bind(local_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

    if (listen(local_sock, 10) == -1) {
		perror("listen error");
		exit(-1);
	}

    /* Assign signal handlers to signals. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signalHandler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    // if (signal(SIGKILL, signalHandler) == SIG_ERR) {
    //     perror("signal");
    //     exit(1);
    // }
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    for(int j = 0; j < FD_SETSIZE; ++j){
        startPlayer[j] = false;
    }
    gameStart = false;

    clilen = sizeof(cli_addr);

    pthreadArgs[numOfThreads].socketFileDes = local_sock;

    if (pthread_create(&pthread[numOfThreads], NULL, acceptLocal, &pthreadArgs[numOfThreads]) != 0) {
        perror("pthread_create");
        // free(pthreadArgs);
        exit(EXIT_FAILURE);
    }

    ++numOfThreads;

    while (1) {
       
        newSocketFileDes = accept(socket_fd, (struct sockaddr *) &cli_addr, &clilen);
        if (newSocketFileDes < 0) {
          close(socket_fd);
          perror("accept");
          // free(pthreadArgs);
          exit(EXIT_FAILURE);
        }else{
          printf("Connected to Client!");
        }

        pthread_mutex_lock(&mutex4);
        newGame = !gameStart;
        pthread_mutex_unlock(&mutex4);

        if(newGame){
            printf("\n\nNew game !\n\n");
            if(numOfThreads > 1){

                for(int i = 1; i <numOfThreads; i++){
                    pthread_join(pthread[i], NULL);
                }

                numOfThreads = 1;
            }

            for(int i = 0; i< ROWS; ++i){
                memset(arenaMap[i], ' ', COLS);
            }

            
            if (pthread_create(&pthread[numOfThreads], NULL, spawnFruit, NULL) != 0) {
                perror("pthread_create_fruit");
                // free(pthreadArgs);
                exit(EXIT_FAILURE);
            }

            printf("Spawned Fruit Thread\n");
            ++numOfThreads;
        }

        /* Initialise pthread argument. */
        // pthreadArgs->socketFileDes = socketFileDes;
        pthreadArgs[numOfThreads].socketFileDes = newSocketFileDes;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */

        /* Create thread to serve connection to client. */
        if (pthread_create(&pthread[numOfThreads], NULL, clientThread, &pthreadArgs[numOfThreads]) != 0) {
            perror("pthread_create");
            // free(pthreadArgs);
            continue;
        }

        ++numOfThreads;

        
        if (numOfThreads >= MAXTHREADS) {
            // Update i
            numOfThreads = 1;
 
            while (numOfThreads < MAXTHREADS) {
                
                pthread_join(pthread[numOfThreads++],
                             NULL);
            }
 
            // Update i
            numOfThreads = 1;
        }
    }

    /* close(socket_fd);
     * TODO: If you really want to close the socket, you would do it in
     * signalHandler(), meaning socket_fd would need to be a global variable.
     */
    return 0;
}

void *acceptLocal(void *arg){
    bool newGame;
    int newSocketFileDes;
    pthreadArgument *pthreadArgs = (pthreadArgument *)arg;
    int local_sock = pthreadArgs->socketFileDes;
   

    while (1) {
    
        newSocketFileDes = accept(local_sock, NULL, NULL);
        if (newSocketFileDes < 0) {
          close(local_sock);
          perror("accept");
          pthread_exit(NULL);
        }else{
          printf("Connected to Client!");
        }

        pthread_mutex_lock(&mutex4);
        newGame = !gameStart;
        pthread_mutex_unlock(&mutex4);

        if(newGame){
            printf("\n\nNew game !\n\n");
            if(numOfThreads > 1){

                for(int i = 1; i <numOfThreads; i++){
                    pthread_join(pthread[i], NULL);
                }

                numOfThreads = 1;

            }

            for(int i = 0; i< ROWS; ++i){
                memset(arenaMap[i], ' ', COLS);
            }

            
            if (pthread_create(&pthread[numOfThreads], NULL, spawnFruit, NULL) != 0) {
                perror("pthread_create_fruit");
                // free(pthreadArgs);
                exit(EXIT_FAILURE);
            }

            printf("Spawned Fruit Thread\n");
            ++numOfThreads;
        }

        pthreadArgs[numOfThreads].socketFileDes = newSocketFileDes;
    
        if (pthread_create(&pthread[numOfThreads], NULL, clientThread, &pthreadArgs[numOfThreads]) != 0) {
            perror("pthread_create");
            continue;
        }

        ++numOfThreads;

        
        if (numOfThreads >= MAXTHREADS) {
            // Update i
            numOfThreads = 1;
 
            while (numOfThreads < MAXTHREADS) {
                
                pthread_join(pthread[numOfThreads++],
                             NULL);
            }
 
            // Update i
            numOfThreads = 1;
        }
    }
}

void *spawnFruit(void* arg){

    bool valid = false;
    int x,y;
    int delay;
    int keepLooping;
    int numOfFruit = 0;
    

    sleep(10);
    keepLooping = true;
    
    do{

        numOfFruit = 0;
        valid = false;

        while(!valid){
            //numOfFruit = 0;
            x = random()%(COLS -2) + 1;
            y = random()%(ROWS - 2) + 1;
            
            valid = true;
            
            //---------------------------------------------------------
            pthread_mutex_lock(&mapMutex);
            if(arenaMap[y][x] == ' ')
                arenaMap[y][x] = '*';
            else valid = false;
            pthread_mutex_unlock(&mapMutex);
            //----------------------------------------------------------
        }

        for(int i = 0; i < MAXFRUIT; ++i){
            numOfFruit++;
            pthread_mutex_lock(&mutex3);
            if(strlen(fruitCoordinates[i]) == 0){
                sprintf(fruitCoordinates[i], "%d,%d", x, y);
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

    for(int i = 0; i < MAXFRUIT; ++i){
        pthread_mutex_lock(&mutex3);
        if(strlen(fruitCoordinates[i]) > 0){
            strcpy(fruitCoordinates[i], "\0");
        }
        pthread_mutex_unlock(&mutex3);
    }

    pthread_exit(NULL);

}



// Thread for clients
// Contains game logic
void *clientThread(void *arg) {
    pthreadArgument *pthreadArgs = (pthreadArgument *)arg;
    int socketFileDes = pthreadArgs->socketFileDes;

    printf("Inside Thread with socket: %d\n,", socketFileDes);

    char buffer[MAXMSG];
    char temp[MAXMSG];
    int nbytes;
    char *values;
    bool wallHit;
    bool valid;
    int x,y;
    int length;
    int currDir;
    Snake *snake;
    Snake *head;
    int newx, newy;
    int gameOver;
    bool eaten;
    int points;
    bool gameWon;
    bool disconnect;

    // start snake
    if (readSnakeCommand(socketFileDes) < 0) {
        close(socketFileDes);
        printf("Exiting thread: %d\n", socketFileDes);
        pthread_exit(NULL);
    }

    // Set rows and columns into buffer
    memset(buffer, 0, MAXMSG);
    sprintf(buffer, "%d,%d,", ROWS, COLS);

    // Send rows and columns to client
    if ((nbytes = write(socketFileDes, buffer, strlen(buffer))) < 0) {
        close(socketFileDes);
        perror("ERROR writing to socket");
        pthread_exit(NULL);
    }

    printf("Sent rows and cols\n");

    // Reset buffer
    memset(buffer, 0, MAXMSG);

    valid = false;

    // Spawn snake onto map (keep looping till valid position found)  
    while(!valid){

        // Generate random playerCoordinates for the tail
        x = random()%(COLS -2) + 1;
        y = random()%(ROWS - 2) + 1;
        valid = true;

        // Mutex lock to access and modify current view of map
        pthread_mutex_lock(&mapMutex);
        // If empty space
        if(arenaMap[y][x] == ' '){
            
            // Check if there's space of up to 5 spaces ahead of tail in the arena (2 for the rest of the body, 3 for movement) 
            //      and make sure first space ahead of snake is not occupied.
            // In case space is tight, spawning of the rest of the snake can replace fruit as well. 
            // Alternate directions of movement until one viable is found.
            if((y + 5) < (ROWS) && arenaMap[y + 1][x] != '#' && arenaMap[y + 2][x] != '#' && arenaMap[y + 3][x] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = down;
                // sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x, y+1, x, y+2);

            }else if((y - 5) > 0 && arenaMap[y - 1][x] != '#' && arenaMap[y - 2][x] != '#' && arenaMap[y - 3][x] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y-1][x] = '#';
                arenaMap[y-2][x] = '#';
                currDir = up;
                // sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x, y-1, x, y-2);

            }else if((x + 5) < (COLS) && arenaMap[y][x+1] != '#' && arenaMap[y][x+2] != '#' && arenaMap[y][x+3] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = right;
                // sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x+1, y, x+2, y);

            }else if((x - 5) > 0 && arenaMap[y][x-1] != '#' && arenaMap[y][x-2] != '#' && arenaMap[y][x-3] != '#'){
                arenaMap[y][x] = '#';
                arenaMap[y+1][x] = '#';
                arenaMap[y+2][x] = '#';
                currDir = left;
                // sprintf(buffer, "!%d,%d,%d,%d,%d,%d", x, y, x-1, y, x-2, y);

            }else valid = false;

        }else valid = false;

        pthread_mutex_unlock(&mapMutex);
        
    }

    // Initialize snake structure with tail playerCoordinates and direction
    snake = initializeSnake(x, y, currDir);
    // Retrieve the head of the tail
    head = getHead(snake);

    // Set playerCoordinates of the head
    x = head->x;
    y = head->y;

    printf("Computed x and y\n");

    
    // Create new entry in playerCoordinates for socket
    pthread_mutex_lock(&mutex1);
    playerCoordinates[socketFileDes] = (char *)malloc(MAXMSG * sizeof(char));
    // strcpy(playerCoordinates[socketFileDes], buffer);
    pthread_mutex_unlock(&mutex1);

    printf("Allocated x and y\n");

    // put head playerCoordinates and starting direction in buffer
    memset(buffer, 0, MAXMSG);
    sprintf(buffer, "%d,%d,%d,", x, y, currDir);

    // copy current image of the map into buffer as well
    pthread_mutex_lock(&mapMutex);
    strcat(buffer, arenaMap[0]);
    pthread_mutex_unlock(&mapMutex);
    
    // Send buffer to client
    if ((nbytes = write(socketFileDes, buffer, strlen(buffer))) < 0) {
        close(socketFileDes);
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }

    printf("Sent x, y, direction and map\n");

    
    // Initialize
    newx = x;
    newy = y;
    eaten = false;
    wallHit = false;
    points = 0;
    gameWon = false;
    gameOver = false;
    disconnect = false;

    // Main game loop
    while(!gameOver){

        // Assign tail part to x and y
        x = snake->x;
        y = snake->y;

        memset(buffer, 0, MAXMSG);
        eaten = false;

        // Read direction from client
        if ((nbytes = read(socketFileDes, &currDir, sizeof(int))) < 0) {
            // close(socketFileDes);
            // perror("ERROR read");
            // exit(EXIT_FAILURE);
        }

        // If Access to client denied, end the game due to disconnection
        if(nbytes <= 0){

            gameOver = true;
            disconnect = true;
        }

        // Increment/decrement playerCoordinates based on movement, making sure not to crash into arena boundaries
        switch(currDir){
            case up: 
                if ( newy > 1){
		            --newy;
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
                break;

            case down:
                if ( newy < ROWS - 2){
		            ++newy;
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
	            break;

            case left:
                if ( newx > 1 ){
		            --newx;
                }
                else{
                    gameOver = true; 
                    wallHit = true;
                }
	            break;

            case right:
                if ( newx < COLS - 2){
		            ++newx;
                }
                else {
                    gameOver = true; 
                    wallHit = true;
                }
	            break;

            default: break;
        }

        //check game affecting conditions

        // Check collision
        pthread_mutex_lock(&mapMutex);
        if(arenaMap[newy][newx] == '#'){
            gameOver = true;
            wallHit = true;

        // Check for fruit
        }else if(arenaMap[newy][newx] == '*'){
            eaten = true;
            points++;
            
            if(points == 12){
                gameWon = true;
                gameOver = true;
                pthread_mutex_lock(&mutex4);
                gameStart = false;
                pthread_mutex_unlock(&mutex4);
            }

        // See if someone won, hence ending the game
        }else{
            pthread_mutex_lock(&mutex4);
            if(!gameStart) {
                gameOver = true;
                wallHit = true;
            }
            pthread_mutex_unlock(&mutex4);
        }
        pthread_mutex_unlock(&mapMutex);
        

        // If eaten fruit, remove fruit from fruit list
        if(!gameOver && eaten){
            for (int i = 0; i < MAXFRUIT; ++i){
                pthread_mutex_lock(&mutex3);
                length = strlen(fruitCoordinates[i]);
                pthread_mutex_unlock(&mutex3);

                if(length > 0){
                    memset(buffer, 0, MAXMSG);

                    pthread_mutex_lock(&mutex3);
                    strcpy(buffer, fruitCoordinates[i]);
                    pthread_mutex_unlock(&mutex3);

                    char *temp;
                    int fruitX;
                    int fruitY;

                    temp = strtok(buffer, ",");
                    fruitX = atoi(temp);
                    temp = strtok(NULL, ",");
                    fruitY = atoi(temp);

                    // If current playerCoordinates match of fruit, remove
                    if(newx == fruitX && newy == fruitY){
                        //eaten = true;
                        pthread_mutex_lock(&mutex3);
                        strcpy(fruitCoordinates[i], "\0");
                        pthread_mutex_unlock(&mutex3);
                        break;
                    }
                }
            }

        }

        // Update values of shared variable for this snake
        // If game is over, share all playerCoordinates of snake to be removed
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

        // If eaten fruit, share new head of snake to be added
        }else if(eaten){
            sprintf(buffer, "+%d,%d", newx, newy);

            pthread_mutex_lock(&mapMutex);
            arenaMap[newy][newx] = '#';
            pthread_mutex_unlock(&mapMutex);
            
            // Add a new head
            snake = addPart(newx, newy, snake);

        // Otherwise, simply share old coordinate of tail to be removed, as well as new coordinate of head to be added.
        }else{
            sprintf(buffer, "%d,%d,%d,%d", x, y, newx, newy);

            pthread_mutex_lock(&mapMutex);
            arenaMap[newy][newx] = '#';
            arenaMap[y][x] = ' ';
            pthread_mutex_unlock(&mapMutex);
            
            // Move snake
            snake = moveSnake(newx, newy, &snake);
        }

        // copy new values to shared variable
        pthread_mutex_lock(&mutex1);
        strcpy(playerCoordinates[socketFileDes], buffer);
        pthread_mutex_unlock(&mutex1);
        
        // Delay for 0.25 seconds to allow values to be updated in other threads 
        usleep(250000);

        // If the client hasn't disconnected, send all information on other clients to current client
        if(!disconnect){
            if (writeAllToClient(socketFileDes) < 0) {
                close(socketFileDes);
                disconnect = true;
                //pthread_exit(NULL);

            }
        }

        // Check if global game has ended
        pthread_mutex_lock(&mutex4);
        if(!gameOver) gameOver =  !gameStart;
        pthread_mutex_unlock(&mutex4);
    }

    // Sleep to allow time for other clients to acquire terminated player information
    sleep(2);

    // Indicate player is terminated
    pthread_mutex_lock(&mutex2);
    startPlayer[socketFileDes] = false;

    pthread_mutex_lock(&mutex4);
    // If game hasn't ended, check if other players are alive. If not, end game.
    if(gameStart){
        gameStart = false;
        for (int i = 0; i < FD_SETSIZE; ++i){
            if(startPlayer[i]){
                gameStart = true;
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex4);
    pthread_mutex_unlock(&mutex2);

    memset(buffer, 0, MAXMSG);

    // Send final message to client to indicate whether they won or lost
    if(gameWon){
        strcpy(buffer, "winner");

        if ((nbytes = write(socketFileDes, buffer, strlen(buffer))) < 0) {
            close(socketFileDes);
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
    
    // If client has not disconnected, send message
    }else if(!disconnect){
        strcpy(buffer, "loser");

        if ((nbytes = write(socketFileDes, buffer, strlen(buffer))) < 0) {
            close(socketFileDes);
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
    }

    // Clean up
    printf("Exiting Thread %d\n", socketFileDes);
    close(socketFileDes);
    pthread_exit(NULL);
}

int readSnakeCommand(int filedes) {

    char buffer[MAXMSG];
    int nbytes;
    memset(buffer, 0, MAXMSG);

    nbytes = read(filedes, buffer, MAXMSG);

    if (nbytes < 0) {
        // Read error
        // perror("read");
        // close(filedes);
        // exit(EXIT_FAILURE);
        return -1;
    } 
    else if (nbytes == 0)
        // End-of-file.
        return -1;
    else {
        // Data read.
        fprintf(stderr, "Server got message: %s\n", buffer);

        if(strcmp(buffer, "snake\n") == 0){

            buffer[5] = '\0';
          
            pthread_mutex_lock(&mutex2);
            startPlayer[filedes] = true;
            pthread_mutex_unlock(&mutex2);
            
            pthread_mutex_lock(&mutex4);
            if(!gameStart) gameStart = true;
            pthread_mutex_unlock(&mutex4);

        }else return -1;
       
        return 0;
    }
}

int writeAllToClient(int filedes) {

    int nbytes;
    char buffer[MAXMSG];
    memset(buffer, 0, MAXMSG);
    int length = 0;

    memset(buffer, 0, MAXMSG);

    for (int i = 0; i < FD_SETSIZE; ++i){
        pthread_mutex_lock(&mutex2);
        if (startPlayer[i]) {
            //buffer = (char *)realloc( buffer, (strlen(buffer) + strlen(values[filedes])) * sizeof(char));
            //sprintf(temp, "%d, ", i);

            pthread_mutex_lock(&mutex1);
            strcat(buffer, playerCoordinates[i]);
            pthread_mutex_unlock(&mutex1);
            strcat(buffer, "|");
            
        }
        pthread_mutex_unlock(&mutex2);
    }

    strcat(buffer, "$");

    for (int i = 0; i < MAXFRUIT; ++i){
        pthread_mutex_lock(&mutex3);
        length = strlen(fruitCoordinates[i]);
        pthread_mutex_unlock(&mutex3);

        if(length > 0){

            pthread_mutex_lock(&mutex3);
            strcat(buffer, fruitCoordinates[i]);
            pthread_mutex_unlock(&mutex3);
            strcat(buffer, ",");
        }
    }

     
    if ((nbytes = write(filedes, buffer, strlen(buffer))) < 0) {
        return -1;
        // close(filedes);
        // perror("ERROR writing to socket");
        // exit(EXIT_FAILURE);
    }

    return 0;
}



void signalHandler(int signal) {
    
    close(pthreadArgs[0].socketFileDes - 1);
    for(int i = 0; i < numOfThreads; ++i){
        close(pthreadArgs[i].socketFileDes);
    }

    exit(0);
}
