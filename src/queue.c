
#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


typedef struct QueueElement {
    void *item;
    struct QueueElement *next;

} QueueElement;

struct QueueStruct {
    //pointers to the head and tail of the queue itself
    QueueElement* first;
    QueueElement* last;

    pthread_mutex_t lock; //mutex to ensure only one thread can access the queue at a time
    sem_t spaces_available; //semaphores representing spaces filled and spaces available
    sem_t spaces_taken;
};


/*
 * new_queue_element:
 *
 * Allocates memory for and returns a new QueueElement containing a pointer to
 * some arbitrary data (the queue is type-agnostic) and a pointer to the next element
 * in the queue, initially null.
 */
QueueElement* new_queue_element(void* item) {
    QueueElement* element = malloc(sizeof(QueueElement));
    element->item = item;
    element->next = NULL;

    return element;
}


Queue *queue_alloc(int size) {
    Queue* queue = malloc(sizeof(Queue));
    queue->first = NULL;
    queue->last = NULL;

    //initialize required synchronization primitives
    pthread_mutex_init(&(queue->lock), NULL);
    sem_init(&(queue->spaces_available), 0, size);
    sem_init(&(queue->spaces_taken), 0, 0);

    return queue;
}

/*
 * queue_element_free:
 *
 * Frees a dynamically allocated QueueElement from memory.
 * The contained item is freed first so that its memory does not become
 * inaccessible.
 */
void queue_element_free(QueueElement* element) {
    free(element->item);
    free(element);
}


void queue_free(Queue *queue) {
    QueueElement *previous, *current;

    for (current = queue->first; current; current = current->next) {
        previous = current;
        queue_element_free(previous); //free each trailing QueueElement as the queue is traversed

    } free(queue);
}


void queue_put(Queue *queue, void *item) {
    sem_wait(&(queue->spaces_available)); //wait until there is a space in the queue
    pthread_mutex_lock(&(queue->lock)); //entering critical region

    if (queue->first) { //if the queue has a first element then it has a last element
        queue->last->next = new_queue_element(item);
        queue->last = queue->last->next;

    } else { //empty queue - newly inserted item is both the first and last element
        queue->first = new_queue_element(item);
        queue->last = queue->first;
    }

    pthread_mutex_unlock(&(queue->lock)); //exiting critical region
    sem_post(&(queue->spaces_taken)); //one more space is now occupied
}


void *queue_get(Queue *queue) {
    sem_wait(&(queue->spaces_taken)); //block until the queue is non-empty
    pthread_mutex_lock(&(queue->lock)); //entering critical region

    QueueElement *outgoing_element = queue->first;
    void *item = outgoing_element->item;
    queue->first = queue->first->next;

    pthread_mutex_unlock(&(queue->lock)); //exiting critical region
    sem_post(&(queue->spaces_available)); //there is now one more space in the queue

    free(outgoing_element); //cannot free the contained item as it is still needed
    return item;
}
