#include "playerSnake.h"
#include <stdlib.h>

//Initialize list to head node
Snake *initializeSnake(WINDOW* win, int x, int y){
    Snake *head = NULL;

    head = (Snake *) malloc(sizeof(Snake));
    if(head == NULL){
        printf("Out of memory\n");
        return NULL;
    }

    head->x = x;
    head->y = y;
    mvwaddch(win, y, x, '#');

    head = addPart(win, x, ++y, head);
    head = addPart(win, x, ++y, head);

    // head->nextPart = (Snake *) malloc(sizeof(Snake));
    // head->x = x;
    // head->y = ++y;
    // mvwaddch(win, y, x, '#');

    // for(int i = 0; i < 3; i++){
    //     if(mvwinch(win, y, x) == ' ')
    // }

    // if(mvwinch(win, y, x) == ' '){
    //     mvwaddch(win, y, x, '#');

    // }
    

    return head;
}

Snake *addPart(WINDOW* win, int x, int y, Snake* head){

    Snake * prev, * current;

    current = head;

    //Traverse to end of list
    while(current != NULL){
        prev = current;
        current = current-> nextPart;
    }

    //Create new node
    if(current == NULL){
        current = (Snake *) malloc(sizeof(Snake));
        prev->nextPart = current;
        current->nextPart = NULL;
        current->x = x;
        current->y = y;
        mvwaddch(win, y, x, '#');
    }

    return head;
}

Snake *deletePart(Snake **headRef, int part, WINDOW* win){ 
    // Store head node 
    Snake* temp = *headRef, *prev; 
    int i = 0;
    //Locate pid
    while (temp != NULL && part != i) 
    { 
        prev = temp; 
        temp = temp->nextPart; 
        i++;
    } 
  
    // If pid not found
    if (temp == NULL){
        printf("Could not find PID\n");
        return NULL;
    } 
     
  
    // Unlink the node from linked list 
    prev->nextPart = temp->nextPart; 
  
    free(temp); 

    //Return next node
    return prev->nextPart;
}


Snake* moveSnake(WINDOW* win, int x, int y, Snake** head){
    Snake *prev, *current, *newTail;

    
    current = *head;
    newTail = (*head)->nextPart;
    mvwaddch(win, current->y, current->x, ' ');

    //Traverse to end of list
    while(current != NULL){
        prev = current;
        current = current-> nextPart;
    }

    //Create new node
    if(current == NULL){
        // current = (Snake *) malloc(sizeof(Snake));
        // prev->nextPart = current;
        prev->nextPart = *head;
        (*head)->x = x;
        (*head)->y = y;
        (*head)->nextPart = NULL;
        // head->nextPart = NULL;
        mvwaddch(win, y, x, '#');
    }

    return newTail;

}

Snake *getTail(WINDOW* win, Snake* head){

    Snake * prev, * current;

    current = head;

    //Traverse to end of list
    while(current != NULL){
        prev = current;
        current = current-> nextPart;
    }

    //Create new node
    return prev;
}


