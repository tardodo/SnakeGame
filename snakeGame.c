#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "playerSnake.h"
 
enum direction{up, down, left, right};

int currDir = down;

typedef struct{
    int x;
    int y;
}Consumable;

Snake* snake;

// typedef struct snakePart{
//     int x;
//     int y;
//     struct snakePart* nextPart;
// }Snake;

// void moveSnake(WINDOW* win, int direction){
//     switch(currDir){
//         case up:

//     }
//     // if((currDir == up && direction != down) || (currDir == down && direction != up) || (currDir == left && direction != right) || (currDir == right && direction != left)){
//     //     currDir = direction;
//     //     switch (currDir)
//     //     {
//     //     case /* constant-expression */:
//     //         /* code */
//     //         break;
        
//     //     default:
//     //         break;
//     //     }
//     // }
// }

void spawnCons(WINDOW* win, int maxX, int maxY){
    int x;
    int y;
    chtype check;
    bool valid = false;

    while(!valid){

        x = random()%maxX;
        y = random()%maxY; 

        if(x > 1 && y > 1){
            if((check = mvwinch(win, y, x)) == ' ' ){
                mvwaddch(win, y, x, '*');
                valid = true;
            }
        }
    }
}

int main(void) {
    
    initscr();
    refresh();
    cbreak();
    noecho();

    signal (SIGWINCH, NULL);
    curs_set(0);

    WINDOW* arena;
 
 
    int      width = 20, height = 20;
    int      rows  = width, cols   = width*2.5;
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int ch;
    // int y = (maxY/2) - (rows/2) + 5, x = (maxX/2) - (cols/2) + 5;
    int y = 1, x = 1;
    // int currDir = down;
    bool gameOver = false;
    bool eaten = false;
    
    
    //box(mainwin, 0, 0);
    
    arena = newwin(rows, cols, (maxY/2) - (rows/2), (maxX/2) - (cols/2));
    // wtimeout(arena, 500);
    nodelay(arena, true);
    
    wclear(arena);
    box(arena, 0, 0);
    keypad(arena, TRUE);
    int arenaMaxY, arenaMaxX;
    getmaxyx(arena, arenaMaxY, arenaMaxX);

    // refresh
    wrefresh(arena);


    // wgetch(arena);
    //mvwaddch(arena, y, x, 'a');
    snake = initializeSnake(arena, x, y);
    Snake* tail = getTail(arena, snake);
    x = tail->x;
    y = tail->y;

    spawnCons(arena, arenaMaxX-2, arenaMaxY-2);
    //mvaddch(y-5, x-5, 'a');
    while(!gameOver){
        // usleep(1000000);
        if(eaten) {
            spawnCons(arena, arenaMaxX-2, arenaMaxY-2);
            eaten = false;
        }
        // flushinp();
        
        // clock_t start = clock();
        
        if((ch = wgetch(arena)) != ERR){
            switch(ch){
                case 'w': if(currDir != down && currDir != up) currDir = up; break;
                case 's': if(currDir != up && currDir != down) currDir = down;break;
                case 'a': if(currDir != right && currDir != left) currDir = left;break;
                case 'd': if(currDir != left && currDir != right) currDir = right;break;
                default: break;
            }
        }
        
        // clock_t end = clock();
        // float seconds = (float)(end - start) / CLOCKS_PER_SEC;
        
        // if(seconds < 0.5){
        //     float remaining = 0.5 - seconds;
        //     usleep(remaining*500000);
        // }

        flushinp();
        //wclear(arena);
        // wrefresh(arena);

        switch(currDir){
            case up: 
                if ( y > 1){
                    //mvwaddch(arena, y, x, ' ');
		            --y;
                    
                }else gameOver = true;
                break;

            case down:
                if ( y < arenaMaxY - 2){
                    //mvwaddch(arena, y, x, ' ');
		            ++y;
                }else gameOver = true;
	        break;

            case left:
                if ( x > 1 ){
                    //mvwaddch(arena, y, x, ' ');
		            --x;
                }else gameOver = true;
	        break;

            case right:
                if ( x < arenaMaxX - 2){
                    //mvwaddch(arena, y, x, ' ');
		            ++x;
                }else gameOver = true;
	        break;

            default: break;
        }

        if(mvwinch(arena, y, x) == '*'){
            snake = addPart(arena, x, y, snake);
            eaten = true;
        }else if(mvwinch(arena, y, x) == '#'){
            gameOver = true;

        }else snake = moveSnake(arena, x, y, &snake);

        //mvwaddch(arena, y, x, 'a');
        wrefresh(arena);

        usleep(500000);

    }
    

    

    delwin(arena);
    endwin();
    // refresh();
 
    return EXIT_SUCCESS;
}