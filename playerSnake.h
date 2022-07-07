// #include <ncurses.h>

typedef struct snakePart{
    int x;
    int y;
    struct snakePart* nextPart;
}Snake;

Snake *initializeSnake(int x, int y, int dir);

Snake *addPart(int x, int y, Snake* tail);

Snake* moveSnake(int x, int y, Snake** tail);

Snake *getHead(Snake* tail);
