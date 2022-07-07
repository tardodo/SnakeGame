#include "playerSnake.h"
#include <stdlib.h>
#include <stdio.h>

enum direction{up, down, left, right};

//Initialize list to tail node
Snake *initializeSnake(int x, int y, int dir){
    Snake *tail = NULL;

    tail = (Snake *) malloc(sizeof(Snake));
    if(tail == NULL){
        printf("Out of memory\n");
        return NULL;
    }

    tail->x = x;
    tail->y = y;

    if(dir == down){
        tail = addPart(x, ++y, tail);
        tail = addPart(x, ++y, tail);
    }else if(dir == up){
        tail = addPart(x, --y, tail);
        tail = addPart(x, --y, tail);
    }else if(dir == left){
        tail = addPart( --x, y, tail);
        tail = addPart( --x, y, tail);
    }else if(dir == right){
        tail = addPart(++x, y, tail);
        tail = addPart(++x, y, tail);
    }

    return tail;
}

Snake *addPart(int x, int y, Snake* tail){

    Snake * prev, * current;

    current = tail;

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
    }

    return tail;
}

Snake *deletePart(Snake **tailRef, int part){ 
    // Store tail node 
    Snake* temp = *tailRef, *prev; 
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
        printf("Could not find part\n");
        return NULL;
    } 
     
  
    // Unlink the node from linked list 
    prev->nextPart = temp->nextPart; 
  
    free(temp); 

    //Return next node
    return prev->nextPart;
}


Snake* moveSnake(int x, int y, Snake** tail){
    Snake *prev, *current, *newTail;

    current = *tail;
    newTail = (*tail)->nextPart;

    //Traverse to end of list
    while(current != NULL){
        prev = current;
        current = current-> nextPart;
    }

    //Create new node
    if(current == NULL){
        prev->nextPart = *tail;
        (*tail)->x = x;
        (*tail)->y = y;
        (*tail)->nextPart = NULL;
    }

    return newTail;

}

Snake *getHead(Snake* tail){

    Snake * prev, * current;

    current = tail;

    //Traverse to end of list
    while(current != NULL){
        prev = current;
        current = current-> nextPart;
    }

    //Create new node
    return prev;
}


