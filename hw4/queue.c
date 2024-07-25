#include "queue.h"
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
    mtx_t primary_mutex;
    mtx_t secondary_mutex;
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
 * queue is empty!
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
    mtx_init(&queue.primary_mutex, mtx_plain);
    mtx_init(&queue.secondary_mutex, mtx_plain);
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
    free_cond_queue(&queue.waiting_queue);
    destroy_simple_queue(&queue.waiting_queue);
    destroy_simple_queue(&queue.data_queue);
}

void enqueue(void *item) {
    mtx_lock(&queue.primary_mutex);
    simple_enqueue(&queue.data_queue, item);
    release_waiting_on_queue();
    mtx_unlock(&queue.primary_mutex);
}

/**
 * Blocks until a new item is enqueued. queue.mutex should be held
 * when this is called, and is still held when this returns.
 * */
void wait_on_queue(void) {
    cnd_t *cond = (cnd_t *)malloc(sizeof(cnd_t));
    cnd_init(cond);
    simple_enqueue(&queue.waiting_queue, (void *)cond);
    cnd_wait(cond, &queue.secondary_mutex);
}

/**
 * Release the next `dequeue` waiting for an item. queue.primary_mutex should be
 * held. Does nothing if there are no threads waiting.
 * */
void release_waiting_on_queue(void) {
    if (is_empty(&queue.waiting_queue)) {
        return;
    }

    cnd_t *cond = simple_dequeue(&queue.waiting_queue);
    cnd_signal(cond);
    cnd_destroy(cond);
    free(cond);
}

void *dequeue(void) {
    mtx_lock(&queue.primary_mutex);
    mtx_lock(&queue.secondary_mutex);
    mtx_unlock(&queue.primary_mutex);
    if (is_empty(&queue.data_queue)) {
        // Queue is empty, wait until an item arrives.
        wait_on_queue();
    }
    // Queue isn't empty!
    void *item = simple_dequeue(&queue.data_queue);
    mtx_unlock(&queue.secondary_mutex);
    return item;
}

bool tryDequeue(void **item) {
    bool result = false;
    mtx_lock(&queue.primary_mutex);
    mtx_lock(&queue.secondary_mutex);
    if (!is_empty(&queue.data_queue)) {
        result = true;
        *item = simple_dequeue(&queue.data_queue);
    }
    mtx_unlock(&queue.secondary_mutex);
    mtx_unlock(&queue.primary_mutex);
    return result;
}

size_t size(void) { return queue.data_queue.size; }

size_t waiting(void) { return queue.waiting_queue.size; }

size_t visited(void) { return queue.data_queue.visited; }
