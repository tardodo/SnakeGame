#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>

#define MAXMSG 2048

// Socket path
char *socket_path = "\0hidden";
enum direction{up, down, left, right};

int currDir;

int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_un addr;
 	char buffer[MAXMSG];
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


    do{
        memset(buffer, 0, MAXMSG);
        fgets(buffer, MAXMSG, stdin);
    }while(strcmp(buffer, "snake\n") != 0);
    
    //strcpy(buffer, "snake");
    if ((n = write(fd, buffer, strlen(buffer))) < 0)
    {
         perror("ERROR writing to socket");
		    close(fd);
         exit(1);
    }

    memset(buffer, 0, MAXMSG);

    if ((n = read(fd, buffer, MAXMSG)) < 0)
    {
         perror("ERROR reading from socket");
		      close(fd);
         exit(1);
    }

    // printf("%s\n", buffer);

    int sizeOfMsg = strlen(buffer);
    char *temp;
    int rows;
    int cols;
    int maxY, maxX;
    int x, y;
    int currentPlayerX, currentPlayerY;

    temp = strtok(buffer, ",");
    rows = atoi(temp);
    temp = strtok(NULL, ",");
    cols = atoi(temp);
    temp = strtok(NULL, ",");
    if(sizeOfMsg > 6){
        x= atoi(temp);
        temp = strtok(NULL, ",");
        y = atoi(temp);
        temp = strtok(NULL, ",");
        currDir = atoi(temp);
        temp = strtok(NULL, ",");
    }else{
        memset(buffer, 0, MAXMSG);
        if ((n = read(fd, buffer, MAXMSG)) < 0)
        {
            perror("ERROR reading from socket");
		    close(fd);
            exit(1);
        }
        // printf("%s\n", buffer);
        temp = strtok(buffer, ",");
        x= atoi(temp);
        temp = strtok(NULL, ",");
        y = atoi(temp);
        temp = strtok(NULL, ",");
        currDir = atoi(temp);
        temp = strtok(NULL, ",");
    }



    currentPlayerX = x;
    currentPlayerY = y;
    
    // memset(buffer, 0, MAXMSG);

    // if ((n = read(fd, buffer, 255)) < 0)
    // {
    //      perror("ERROR reading from socket");
	// 	      close(fd);
    //      exit(1);
    // }

    // temp = strtok(buffer, ",");
    // x= atoi(temp);
    // temp = strtok(NULL, ",");
    // y = atoi(temp);
    

    // wrefresh(arena);
	
    //printf("%s\n", buffer);

    WINDOW* arena;
    initscr();
    refresh();
    cbreak();
    noecho();
    signal (SIGWINCH, NULL);
    curs_set(0);

    getmaxyx(stdscr, maxY, maxX);

    arena = newwin(rows, cols, (maxY/2) - (rows/2), (maxX/2) - (cols/2));
    nodelay(arena, true);
    wclear(arena);
    box(arena, 0, 0);
    keypad(arena, TRUE);
    int arenaMaxY, arenaMaxX;
    getmaxyx(arena, arenaMaxY, arenaMaxX);

    // refresh
    wrefresh(arena);

    
    for(int i = 1; i < rows - 1; i++){
        for(int j = 1; j <cols -1 ; j++){
            mvwaddch(arena, i, j, temp[i * cols + j]);
        }
    }

    //mvwaddch(arena, y, x, '#');
    //mvwprintw(arena, 1,1, "%d, %d", arenaMaxX, arenaMaxY);
    wrefresh(arena);
    //wgetch(arena);

    // // Send message to the server
    // memset(buffer, 0, 256);
    // if ((n = write(fd, "Received Initial Info\n", 23)) < 0)
    // {
    //      perror("ERROR writing to socket");
	// 	 close(fd);
    //      exit(1);
    // }

    //---------------------------------------------------------------------------

    int newx, newy;
    int ch;
    // currDir = down;
    bool gameOver = false;
    char *snakes;
    char *fruit;
    char *player;
    bool waitForWrite = true;

    while(!gameOver){

        if((ch = wgetch(arena)) != ERR){
            switch(ch){
                case 'w': if(currDir != down && currDir != up) currDir = up; break;
                case 's': if(currDir != up && currDir != down) currDir = down;break;
                case 'a': if(currDir != right && currDir != left) currDir = left;break;
                case 'd': if(currDir != left && currDir != right) currDir = right;break;
                default: break;
            }
        }

        flushinp();

        memset(buffer, 0, MAXMSG);
    if ((n = write(fd, &currDir, sizeof(int))) < 0)
    {
         perror("ERROR writing to socket");
		 close(fd);
         exit(1);
    }

    // sleep(1);

    // // Read response from server response
    // memset(buffer, 0, 256);

        memset(buffer, 0, MAXMSG);
        if ((n = read(fd, buffer, MAXMSG)) < 0)
        {
            perror("ERROR reading from socket");
		    close(fd);
            delwin(arena);
            endwin();
            exit(1);
        }

        if(n == 0){
            break;
        }

        if(strcmp(buffer, "winner") == 0 || strcmp(buffer, "loser") == 0){
            waitForWrite = false;
            gameOver = true;
        }

        if(!gameOver){

        snakes = strtok(buffer, "$");
        fruit = strtok(NULL, "$");
        //printf("%s\n", buffer);
        
        // temp = strtok(snakes, ",");
        player = strtok_r(snakes, "|", &snakes);

        while(player != NULL){
            temp = strtok(player, ",");

            
            if(strchr(temp, '!') != NULL){
                ++temp;
                while(temp != NULL){
                    x= atoi(temp);
                    temp = strtok(NULL, ",");
                    y = atoi(temp);
                    temp = strtok(NULL, ",");
                    mvwaddch(arena, y, x, '#');
                }

            }else if(strchr(temp, '-') != NULL){
                ++temp;

                while(temp != NULL){
                    x= atoi(temp);
                    temp = strtok(NULL, ",");
                    y = atoi(temp);
                    temp = strtok(NULL, ",");
                    mvwaddch(arena, y, x, ' ');

                    if(currentPlayerX == x && currentPlayerY == y){
                        gameOver = true;
                    }
                }

            
            // }else if(strchr(temp, '+') != NULL){
            //     x= atoi(temp + 1);
            //     temp = strtok(NULL, ",");
            //     y = atoi(temp);
            //     temp = strtok(NULL, ",");
            //     mvwaddch(arena, y, x, '#');
            //     if(currentPlayerX == x && currentPlayerY == y){
            //         gameOver = true;
            //     }
            }else if(strchr(temp, '+') != NULL){
                x= atoi(temp + 1);
                temp = strtok(NULL, ",");
                y = atoi(temp);
                temp = strtok(NULL, ",");
                mvwaddch(arena, y, x, '#');
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
            else{
                x= atoi(temp);
                temp = strtok(NULL, ",");
                y = atoi(temp);
                temp = strtok(NULL, ",");
                mvwaddch(arena, y, x, ' ');

                newx= atoi(temp);
                temp = strtok(NULL, ",");
                newy = atoi(temp);
                temp = strtok(NULL, ",");

                // if(currentPlayerX == x && currentPlayerY == y){
                //     currentPlayerX = newx;
                //     currentPlayerY = newy;
                // }

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
            
            player = strtok_r(NULL, "|", &snakes);
        }

        // temp = strtok(player, ",");

        // while(temp != NULL){
        //     if(strchr(temp, '-') != NULL){
        //         x= atoi(temp + 1);
        //         temp = strtok(NULL, ",");
        //         y = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         mvwaddch(arena, y, x, ' ');
        //         if(currentPlayerX == x && currentPlayerY == y){
        //             gameOver = true;
        //         }
        //     // }else if(strchr(temp, '+') != NULL){
        //     //     x= atoi(temp + 1);
        //     //     temp = strtok(NULL, ",");
        //     //     y = atoi(temp);
        //     //     temp = strtok(NULL, ",");
        //     //     mvwaddch(arena, y, x, '#');
        //     //     if(currentPlayerX == x && currentPlayerY == y){
        //     //         gameOver = true;
        //     //     }
        //     }
        //     else{
        //         x= atoi(temp);
        //         temp = strtok(NULL, ",");
        //         y = atoi(temp);
        //         temp = strtok(NULL, ",");
        //         mvwaddch(arena, y, x, ' ');

        //         newx= atoi(temp);
        //         temp = strtok(NULL, ",");
        //         newy = atoi(temp);
        //         temp = strtok(NULL, ",");

        //         if(currentPlayerX == x && currentPlayerY == y){
        //             currentPlayerX = newx;
        //             currentPlayerY = newy;
        //         }

        //         mvwaddch(arena, newy, newx, '#');
        //     }

        // }

        temp = strtok(fruit, ",");
        while(temp != NULL){
            // if(strchr(temp, '-') != NULL){
            //     x= atoi(temp + 1);
            //     temp = strtok(NULL, ",");
            //     y = atoi(temp);
            //     temp = strtok(NULL, ",");
            //     mvwaddch(arena, y, x, ' ');
            //     if(currentPlayerX == x && currentPlayerY == y){
            //         gameOver = true;
            //     }
            // }
            // else{
                x= atoi(temp);
                temp = strtok(NULL, ",");
                y = atoi(temp);
                temp = strtok(NULL, ",");
                mvwaddch(arena, y, x, '*');

            // }

        }
        
        }

        wrefresh(arena);
        // sleep(2);
        sleep(1);
	
    //printf("%s\n", buffer);
    }

    
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

    if(strcmp(buffer, "winner") == 0){
        // strcpy(buffer, "Congratulations! You won!\n\nPress any key to continue...");

        mvprintw((maxY/2), (maxX/2) - (strlen("Congratulations! You won!")/2), "Congratulations! You won!");
        mvprintw((maxY/2) + 2, (maxX/2) - (strlen("Press any key to continue...")/2), "Press any key to continue...");

    }else{
        // strcpy(buffer, "Sorry! You lost!\n\nPress any key to continue...");
        mvprintw((maxY/2), (maxX/2) - (strlen("Sorry! You lost!")/2), "Sorry! You lost!");
        mvprintw((maxY/2) + 2, (maxX/2) - (strlen("Press any key to continue...")/2), "Press any key to continue...");
    }

    close(fd);
    
    refresh();
    getch();

    endwin();

	return 0;
}