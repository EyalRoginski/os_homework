#include "queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct node_t {
    void *value;
    struct node_t *next;
} node_t;

// Not thread-safe on its own
typedef struct simple_queue_t {
    // Note: the start is always a sentinel node, with no real data (that's why
    // it isn't a pointer)
    node_t start;
    node_t *end;
    size_t size;
    size_t visited;
} simple_queue_t;

typedef struct queue_t {
    simple_queue_t data_queue;
    simple_queue_t waiting_queue;
    mtx_t mutex;
    thrd_t manager_thread;
    cnd_t manager_cond;
    bool awake_waiting; // Is there a thread that's passed the dequeue queue and
                        // been woken up, but hasn't finished dequeuing yet?
    bool active;
} queue_t;

queue_t queue;

void init_simple_queue(simple_queue_t *simple_queue);
void simple_enqueue(simple_queue_t *simple_queue, void *item);
void *simple_dequeue(simple_queue_t *simple_queue);
bool is_empty(simple_queue_t *simple_queue);
void wait_on_queue(void);
void release_waiting_on_queue(void);
void free_cond_queue(simple_queue_t *simple_queue);
void destroy_simple_queue(simple_queue_t *simple_queue);
int manager(void *arg);

void init_simple_queue(simple_queue_t *simple_queue) {
    simple_queue->start.next = NULL;
    simple_queue->end = &simple_queue->start;
    simple_queue->size = 0;
    simple_queue->visited = 0;
}

/**
 * Enqueues `item` into the simple_queue.
 * */
void simple_enqueue(simple_queue_t *simple_queue, void *item) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->value = item;
    new_node->next = NULL;
    simple_queue->end->next = new_node;
    simple_queue->end = new_node;
    simple_queue->size++;
}

/**
 * Dequeues an item from the simple_queue, and returns its value. Do not call if
 * queue is empty (segfault)!
 * */
void *simple_dequeue(simple_queue_t *simple_queue) {
    node_t *removed_node = simple_queue->start.next;
    simple_queue->start.next = removed_node->next;
    if (removed_node->next == NULL) {
        // The end was removed
        simple_queue->end = &simple_queue->start;
    }
    void *item = removed_node->value;
    free(removed_node);
    simple_queue->size--;
    simple_queue->visited++;
    return item;
}

/**
 * Checks if the queue is empty.
 * */
bool is_empty(simple_queue_t *simple_queue) {
    return simple_queue->start.next == NULL;
}

void initQueue(void) {
    init_simple_queue(&queue.data_queue);
    init_simple_queue(&queue.waiting_queue);
    mtx_init(&queue.mutex, mtx_plain);
    cnd_init(&queue.manager_cond);
    queue.active = true;
    queue.awake_waiting = false;
    thrd_create(&queue.manager_thread, manager, NULL);
}

void free_cond_queue(simple_queue_t *simple_queue) {
    node_t *node = simple_queue->start.next;
    while (node) {
        cnd_destroy(node->value);
        free(node->value);
        node = node->next;
    }
}
void destroy_simple_queue(simple_queue_t *simple_queue) {
    node_t *node = simple_queue->start.next;
    while (node) {
        node_t *next = node->next;
        free(node);
        node = next;
    }
}

void destroyQueue(void) {
    queue.active = false;
    cnd_signal(&queue.manager_cond);
    thrd_join(queue.manager_thread, NULL);
    mtx_lock(&queue.mutex);
    free_cond_queue(&queue.waiting_queue);
    destroy_simple_queue(&queue.waiting_queue);
    destroy_simple_queue(&queue.data_queue);
    cnd_destroy(&queue.manager_cond);
    mtx_destroy(&queue.mutex);
}

void enqueue(void *item) {
    if (!queue.active) {
        return;
    }
    mtx_lock(&queue.mutex);
    simple_enqueue(&queue.data_queue, item);
    cnd_signal(&queue.manager_cond);
    mtx_unlock(&queue.mutex);
}

/**
 * Enters the queue of threads waiting to dequeue from the data structure, and
 * blocks until it is woken up by the manager thread. queue.mutex should be held
 * when this is called, and is still held when this returns.
 * */
void wait_on_queue(void) {
    cnd_t *cond = (cnd_t *)malloc(sizeof(cnd_t));
    cnd_init(cond);
    cnd_signal(&queue.manager_cond);
    simple_enqueue(&queue.waiting_queue, (void *)cond);
    cnd_wait(cond, &queue.mutex);
}

/**
 * Release the next `dequeue` waiting for an item. queue.mutex should be
 * held. Does nothing if there are no threads waiting, or if there is no data in
 * the queue.
 * */
void release_waiting_on_queue(void) {
    if (!queue.awake_waiting && !is_empty(&queue.data_queue) &&
        !is_empty(&queue.waiting_queue)) {
        cnd_t *cond = simple_dequeue(&queue.waiting_queue);
        cnd_signal(cond);
        queue.awake_waiting = true;
        cnd_destroy(cond);
        free(cond);
    }
}

/**
 * Manager thread function. Waits to be signaled, whereupon it releases a single
 * thread waiting to dequeue.
 * */
int manager(void *arg) {
    mtx_lock(&queue.mutex);
    while (queue.active) {
        release_waiting_on_queue();
        cnd_wait(&queue.manager_cond, &queue.mutex);
    }
    mtx_unlock(&queue.mutex);
    return 0;
}

void *dequeue(void) {
    if (!queue.active) {
        return NULL;
    }
    mtx_lock(&queue.mutex);
    wait_on_queue();
    void *item = simple_dequeue(&queue.data_queue);
    queue.awake_waiting = false;
    cnd_signal(&queue.manager_cond);
    mtx_unlock(&queue.mutex);
    return item;
}

bool tryDequeue(void **item) {
    if (!queue.active) {
        return false;
    }

    mtx_lock(&queue.mutex);
    while (!is_empty(&queue.data_queue) && !is_empty(&queue.waiting_queue)) {
        cnd_signal(&queue.manager_cond);
        mtx_unlock(&queue.mutex);
        mtx_lock(&queue.mutex);
    }
    if (!is_empty(&queue.data_queue)) {
        *item = simple_dequeue(&queue.data_queue);
        mtx_unlock(&queue.mutex);
        return true;
    }
    mtx_unlock(&queue.mutex);
    return false;
}

size_t size(void) { return queue.data_queue.size; }

size_t waiting(void) { return queue.waiting_queue.size; }

size_t visited(void) { return queue.data_queue.visited; }
