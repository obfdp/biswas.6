#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 
  
/* A structure for representing a queue */
struct msgQ {
    int first, last, msgSize; 
    unsigned msgCapacity; 
    int* array; 
}; 
  
  
struct msgQ* createQ(unsigned msgCapacity) { 
    struct msgQ* queue = (struct msgQ*) malloc(sizeof(struct msgQ)); 
    queue->msgCapacity = msgCapacity; 
    queue->first = queue->msgSize = 0;  
    queue->last = msgCapacity - 1;  /* This is important, see the enQueue */
    queue->array = (int*) malloc(queue->msgCapacity * sizeof(int)); 
    return queue; 
} 
  
/* msgQ is full when msgSize is equal to the msgCapacity  */
int isFull(struct msgQ* queue) {  
    return (queue->msgSize == queue->msgCapacity);  
} 
  
/* msgQ is empty when msgSize is 0 */
int isEmpty(struct msgQ* queue) {
    return (queue->msgSize == 0);
} 
  
/* Function for adding an item to the queue.  */ 

void enQueue(struct msgQ* queue, int item) { 
    if (isFull(queue)) 
        return; 
    queue->last = (queue->last + 1)%queue->msgCapacity; 
    queue->array[queue->last] = item; 
    queue->msgSize = queue->msgSize + 1; 
} 
  
/* Function for removing an item from queue. */ 
/* It changes first and msgSize */
int deQueue(struct msgQ* queue) { 
    if (isEmpty(queue)) 
        return INT_MIN; 
    int item = queue->array[queue->first]; 
    queue->first = (queue->first + 1)%queue->msgCapacity; 
    queue->msgSize = queue->msgSize - 1; 
    return item; 
} 
  
/* Function for getting first of queue */
int first(struct msgQ* queue) { 
    if (isEmpty(queue)) 
        return INT_MIN; 
    return queue->array[queue->first]; 
} 
  
/* Function for getting last of queue */
int last(struct msgQ* queue) { 
    if (isEmpty(queue)) 
        return INT_MIN; 
    return queue->array[queue->last]; 
}
