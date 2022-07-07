#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXMSG 2048

// Socket path
char *socketPath = "\0hidden";
enum direction{up, down, left, right};
char *ip = "127.0.0.1";
int portNum = 5001;
int fd;

int currDir;


int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_in addrTCP;
    struct sockaddr_un addrUnix;
 	char buffer[MAXMSG];
	int rc, n;
    struct hostent *server;
    bool unixSock = false;
    bool tcpSock = false;
    int sizeOfMsg = strlen(buffer);
    char *temp;
    int rows;
    int cols;
    int maxY, maxX;
    int x, y;
    int currentPlayerX, currentPlayerY;
    int newx, newy;
    int ch;
    bool gameOver = false;
    char *snakes;
    char *fruit;
    char *player;
    bool waitForWrite = true;
    

	// Check arguments for tcp or unix socket usage
	if (argc > 1){
        if(strcmp(argv[1], "unix") == 0){
            unixSock = true;

            // optional socket path
            if(argc > 2)
                socketPath = argv[2];

        }else if(strcmp(argv[1], "tcp") == 0){
            tcpSock = true;

            // optional ip 
            if(argc > 2){
                ip = argv[2];

                // optional port
                if(argc > 3)
                    portNum = atoi(argv[3]);
            }

        }else{
            fprintf(stderr, "Usage: client <tcp|unix> [<hostname|path> <port>]\n");
            exit(EXIT_FAILURE);
        }

    }else{
        fprintf(stderr, "Usage: client <tcp|unix> [<hostname|path> <port>]\n");
        exit(EXIT_FAILURE);
    }

    //Create TCP connection
    if(tcpSock){

	    // Create socket 
	    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		    perror("socket error");
		    exit(-1);
	    }

        if ((server = gethostbyname(ip)) == NULL) {
            fprintf(stderr, "ERROR, no such host\n");
		    close(fd);
            exit(0);
        }

	    // Set address structure
	    memset(&addrTCP, 0, sizeof(addrTCP));

        addrTCP.sin_family = AF_INET; 
        memcpy(&addrTCP.sin_addr.s_addr,
           server -> h_addr,
           server -> h_length);
        addrTCP.sin_port = htons(portNum);
    

	    // Connect 
	    if (connect(fd, (struct sockaddr*) &addrTCP, sizeof(addrTCP)) == -1) {
		    perror("connect error");
		    exit(-1);
	    }

    // Create Unix connection
    }else if(unixSock){

        // Create socket
	    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		    perror("socket error");
		    exit(-1);
	    }

        memset(&addrUnix, 0, sizeof(addrUnix));
	    addrUnix.sun_family = AF_UNIX;
	
	    // Copy path to if entered
	    if (*socketPath == '\0') {
		    *addrUnix.sun_path = '\0';
		    strncpy(addrUnix.sun_path + 1, socketPath + 1, sizeof(addrUnix.sun_path) - 2);
	    } else {
		    strncpy(addrUnix.sun_path, socketPath, sizeof(addrUnix.sun_path) - 1);
	    }

	    // Connect 
	    if (connect(fd, (struct sockaddr*) &addrUnix, sizeof(addrUnix)) == -1) {
		    perror("connect error");
		    exit(-1);
	    }
    }

    // Enter snake to join game
    do{
        printf("Please enter \"snake\" to initiate/join game: ");
        memset(buffer, 0, MAXMSG);
        fgets(buffer, MAXMSG, stdin);
    }while(strcmp(buffer, "snake\n") != 0);
    
    
    // Send command to server
    if ((n = write(fd, buffer, strlen(buffer))) < 0){
        perror("ERROR writing to socket");
		close(fd);
        exit(1);
    }

    memset(buffer, 0, MAXMSG);

    // Read payload from server
    if ((n = read(fd, buffer, MAXMSG)) < 0){
        perror("ERROR reading from socket");
		close(fd);
        exit(1);
    }

    // First set of data expected to be rows and columns, therefore abstract data from string
    sizeOfMsg = strlen(buffer);
    temp = strtok(buffer, ",");
    rows = atoi(temp);
    temp = strtok(NULL, ",");
    cols = atoi(temp);
    temp = strtok(NULL, ",");

    // If spawn information and map data got received too, continue abstracting
    if(sizeOfMsg > 6){
        x= atoi(temp);
        temp = strtok(NULL, ",");
        y = atoi(temp);
        temp = strtok(NULL, ",");
        currDir = atoi(temp);
        // temp now contains the map data
        temp = strtok(NULL, ",");
        
    // Otherwise, wait for data to be received
    }else{
        memset(buffer, 0, MAXMSG);
        if ((n = read(fd, buffer, MAXMSG)) < 0){
            perror("ERROR reading from socket");
		    close(fd);
            exit(1);
        }
       
        temp = strtok(buffer, ",");
        x= atoi(temp);
        temp = strtok(NULL, ",");
        y = atoi(temp);
        temp = strtok(NULL, ",");
        currDir = atoi(temp);
        // temp now contains the map data
        temp = strtok(NULL, ",");
    }


    // keep track of head of player
    currentPlayerX = x;
    currentPlayerY = y;

    // Initiate ncurses
    WINDOW* arena;
    initscr();
    refresh();
    cbreak();
    noecho();
    signal (SIGWINCH, NULL);
    curs_set(0);

    getmaxyx(stdscr, maxY, maxX);

    // set arena in middle of terminal screen
    arena = newwin(rows, cols, (maxY/2) - (rows/2), (maxX/2) - (cols/2));

    // remove delay from wgetch()
    nodelay(arena, true);
    wclear(arena);

    // set border
    box(arena, 0, 0);
    
    // refresh
    wrefresh(arena);

    // Add all map data to screen
    for(int i = 1; i < rows - 1; i++){
        for(int j = 1; j <cols -1 ; j++){
            mvwaddch(arena, i, j, temp[i * cols + j]);
        }
    }

    wrefresh(arena);

    // main game loop
    while(!gameOver){
        
        // scan for input from user for direction
        if((ch = wgetch(arena)) != ERR){
            switch(ch){
                case 'w': if(currDir != down && currDir != up) currDir = up; break;
                case 's': if(currDir != up && currDir != down) currDir = down;break;
                case 'a': if(currDir != right && currDir != left) currDir = left;break;
                case 'd': if(currDir != left && currDir != right) currDir = right;break;
                default: break;
            }
        }

        // clear buffer
        flushinp();

        memset(buffer, 0, MAXMSG);

        // Send direction to server
        if ((n = write(fd, &currDir, sizeof(int))) < 0){
            perror("ERROR writing to socket");
		    close(fd);
            exit(1);
        }


        memset(buffer, 0, MAXMSG);

        // Read payload from server
        if ((n = read(fd, buffer, MAXMSG)) < 0)
        {
            perror("ERROR reading from socket");
		    close(fd);
            delwin(arena);
            endwin();
            exit(1);
        }

        // If connection to server lost
        if(n == 0){
            break;
        }

        // Check if payload contains game over message
        if(strcmp(buffer, "winner") == 0 || strcmp(buffer, "loser") == 0){
            waitForWrite = false;
            gameOver = true;
        }

        // If game hasn't ended, process payload
        if(!gameOver){
            
            // get snakes string
            snakes = strtok(buffer, "$");

            // get fruit string
            fruit = strtok(NULL, "$");
        
            // get player string
            player = strtok_r(snakes, "|", &snakes);

            // loop to resolve all player coordinates
            while(player != NULL){

                // get coordinates
                temp = strtok(player, ",");

                // Check if coordinates of player need to be removed
                if(strchr(temp, '-') != NULL){

                    // exclude minus
                    ++temp;

                    // loop to snake of player
                    while(temp != NULL){
                        x= atoi(temp);
                        temp = strtok(NULL, ",");
                        y = atoi(temp);
                        temp = strtok(NULL, ",");
                        mvwaddch(arena, y, x, ' ');
                        
                        // check if head matches client. If so, terminate.
                        if(currentPlayerX == x && currentPlayerY == y){
                            gameOver = true;
                        }
                    }
                
                // Check if new piece needs to be added to snake
                }else if(strchr(temp, '+') != NULL){
                    x= atoi(temp + 1);
                    temp = strtok(NULL, ",");
                    y = atoi(temp);
                    temp = strtok(NULL, ",");
                    mvwaddch(arena, y, x, '#');

                    // See if new head of snake is client's
                    if(currentPlayerX == newx){
                        if((currentPlayerY + 1 )== newy || (currentPlayerY - 1 )== newy ){
                            currentPlayerY = newy;
                        }
                    }else if(currentPlayerY == newy){
                        if((currentPlayerX + 1 )== newx || (currentPlayerX - 1 )== newx ){
                            currentPlayerX = newx;
                        }
                    }

                // Otherwise, normal movement
                }else{

                    // old coordinate
                    x= atoi(temp);
                    temp = strtok(NULL, ",");
                    y = atoi(temp);
                    temp = strtok(NULL, ",");
                    mvwaddch(arena, y, x, ' ');

                    // new head
                    newx= atoi(temp);
                    temp = strtok(NULL, ",");
                    newy = atoi(temp);
                    temp = strtok(NULL, ",");

                    mvwaddch(arena, newy, newx, '#');
                
                    // Keep track of new head
                    if(currentPlayerX == newx){
                        if((currentPlayerY + 1 )== newy || (currentPlayerY - 1 )== newy ){
                            currentPlayerY = newy;
                        }
                    }else if(currentPlayerY == newy){
                        if((currentPlayerX + 1 )== newx || (currentPlayerX - 1 )== newx ){
                            currentPlayerX = newx;
                        }
                    }
                }

                // get the next player coordinates
                player = strtok_r(NULL, "|", &snakes);
            }

            // abstract fruit coordinates
            temp = strtok(fruit, ",");
            while(temp != NULL){
                x= atoi(temp);
                temp = strtok(NULL, ",");
                y = atoi(temp);
                temp = strtok(NULL, ",");
                mvwaddch(arena, y, x, '*');
            }

        }
        
        wrefresh(arena);

        // sleep for 0.5 seconds 
        usleep(500000);
        
    }

    // If game over payload wasn't reached beforehand, wait for it now
    if(waitForWrite){
        memset(buffer, 0, MAXMSG);

        if ((n = read(fd, buffer, MAXMSG)) < 0){
            perror("ERROR reading from socket");
		    close(fd);
            delwin(arena);
            endwin();
            exit(1);
        }
    }

    wclear(arena);

    wrefresh(arena);
    delwin(arena);

    // Check results of end of game payload and display end of game screen
    if(strcmp(buffer, "winner") == 0){

        mvprintw((maxY/2), (maxX/2) - (strlen("Congratulations! You won!")/2), "Congratulations! You won!");
        mvprintw((maxY/2) + 2, (maxX/2) - (strlen("Press any key to continue...")/2), "Press any key to continue...");

    }else{
        
        mvprintw((maxY/2), (maxX/2) - (strlen("Sorry! You lost!")/2), "Sorry! You lost!");
        mvprintw((maxY/2) + 2, (maxX/2) - (strlen("Press any key to continue...")/2), "Press any key to continue...");
    }

    close(fd);
    
    refresh();
    getch();
    endwin();

	return 0;
}