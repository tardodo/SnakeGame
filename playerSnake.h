#include <ncurses.h>

typedef struct snakePart{
    int x;
    int y;
    struct snakePart* nextPart;
}Snake;

Snake *initializeSnake(WINDOW* win, int x, int y);

Snake *addPart(WINDOW* win, int x, int y, Snake* head);

Snake* moveSnake(WINDOW* win, int x, int y, Snake** head);

Snake *getTail(WINDOW* win, Snake* head);
