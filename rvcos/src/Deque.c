#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>

Deque *dmalloc() {
  // Allocating the size of the Deque
  // Deque *d = (Deque *)malloc(sizeof(Deque));
  Deque *d;
  // TMemoryPoolIDRef p = malloc(sizeof(TMemoryPoolID));
  uint32_t p = 0;
  RVCMemoryPoolCreate(d, sizeof(Deque), &p);
  RVCMemoryPoolAllocate(p, sizeof(Deque), (void **)&d);
  d->mPoolID = p;
  // free(p);
  if (d != NULL)
    d->head = d->tail = NULL;
  return d;
}

PrioDeque *pdmalloc() {
  PrioDeque *pd;
  uint32_t p = 0;
  RVCMemoryPoolCreate(pd, sizeof(PrioDeque), &p);
  RVCMemoryPoolAllocate(p, sizeof(PrioDeque), (void **)&pd);
  pd->mPoolID = p;
  pd->high = dmalloc();
  pd->norm = dmalloc();
  pd->low = dmalloc();
  return pd;
}

TBDeque *tbmalloc() {
  // Allocating the size of the Deque
  TBDeque *d;
  uint32_t p = 0;
  RVCMemoryPoolCreate(d, sizeof(TBDeque), &p);
  RVCMemoryPoolAllocate(p, sizeof(TBDeque), (void **)&d);
  d->mPoolID = p;
  if (d != NULL)
    d->head = d->tail = NULL;
  return d;
}

int isEmptyTB(volatile TBDeque *d) {
  // Checking if head is null
  if (d->head == NULL)
    return 1;
  return 0;
}

void tb_push_back(volatile TBDeque *d, TextBuffer v) {
  // struct Node *n = (struct Node *)malloc(sizeof(struct Node));
  struct TextNode *n;
  uint32_t p = 0;
  RVCMemoryPoolCreate(n, sizeof(struct TextNode), &p);
  RVCMemoryPoolAllocate(p, sizeof(struct TextNode), (void **)&n);
  n->mPoolID = p;
  if (n == NULL)
    return;
  // Setting the value of the node
  n->val = v;
  n->prev = d->tail;
  n->next = NULL;
  // If the only node then set to first
  if (d->head == NULL) {
    d->head = d->tail = n;
  } else {
    // Set next of tail to node and set tail to n
    d->tail->next = n;
    d->tail = n;
  }
}

TextBuffer tb_pop_front(volatile TBDeque *d) {
  // Get value
  TextBuffer v = d->head->val;
  struct TextNode *n = d->head;
  if (d->head == d->tail) {
    d->head = d->tail = NULL;
  } else
    // Set head to next
    d->head = n->next;
  // free(n);
  RVCMemoryPoolDeallocate(n->mPoolID, n);
  return v;
}

void push_front(volatile Deque *d, TThreadID v) {
  // struct Node *n = (struct Node *)malloc(sizeof(struct Node));
  struct Node *n;
  uint32_t p = 0;
  RVCMemoryPoolCreate(n, sizeof(struct Node), &p);
  RVCMemoryPoolAllocate(p, sizeof(struct Node), (void **)&n);
  n->mPoolID = p;
  if (n == NULL)
    return;
  // Setting the value of the node
  n->val = v;
  // Adding to the beguining
  n->next = d->head;
  n->prev = NULL;
  // If the only node then set to first
  if (d->tail == NULL) {
    d->head = d->tail = n;
  } else {
    // Set prev of head to node and set head to n
    d->head->prev = n;
    d->head = n;
  }
}

void push_back(volatile Deque *d, TThreadID v) {
  // struct Node *n = (struct Node *)malloc(sizeof(struct Node));
  struct Node *n;
  uint32_t p = 0;
  RVCMemoryPoolCreate(n, sizeof(struct Node), &p);
  RVCMemoryPoolAllocate(p, sizeof(struct Node), (void **)&n);
  n->mPoolID = p;
  if (n == NULL)
    return;
  // Setting the value of the node
  n->val = v;
  n->prev = d->tail;
  n->next = NULL;
  // If the only node then set to first
  if (d->head == NULL) {
    d->head = d->tail = n;
  } else {
    // Set next of tail to node and set tail to n
    d->tail->next = n;
    d->tail = n;
  }
}

int isEmpty(volatile Deque *d) {
  // Checking if head is null
  if (d->head == NULL)
    return 1;
  return 0;
}

void removeT(volatile Deque *d, TThreadID v) {
  if (d->head == NULL) {
    return;
  }
  struct Node *n = d->head;

  // if first is head then found
  if (n->val == v) {
    d->head = n->next;
    return;
  }

  // traverse through the deque till thread found
  while (n->next != NULL && n->val != v) {
    n = n->next;
  }
  // if found
  if (n->val == v) {
    // Remove node
    n->prev->next = n->next;
    if (n->next != NULL)
      n->next->prev = n->prev;
    else
      d->tail = n->prev;
  }
  // free(n);
  RVCMemoryPoolDeallocate(n->mPoolID, n);
}

TThreadID pop_front(volatile Deque *d) {
  // Get value
  TThreadID v = d->head->val;
  struct Node *n = d->head;
  if (d->head == d->tail) {
    d->head = d->tail = NULL;
  } else
    // Set head to next
    d->head = n->next;
  // free(n);
  RVCMemoryPoolDeallocate(n->mPoolID, n);
  return v;
}

TThreadID pop_back(volatile Deque *d) {
  TThreadID v = d->tail->val;
  struct Node *n = d->tail;
  if (d->head == d->tail)
    d->head = d->tail = NULL;
  else
    // Set tail to prev
    d->tail = n->prev;
  // free(n);
  RVCMemoryPoolDeallocate(n->mPoolID, n);
  return v;
}

TThreadID front(volatile Deque *d) {
  // Return head
  return d->head->val;
}

TThreadID end(volatile Deque *d) {
  // Return tail
  return d->tail->val;
}

void writei(uint32_t, uint32_t);

// Debug function to print the deque
void print(volatile Deque *d, uint32_t line) {
  if (d->head == NULL) {
    return;
  }
  struct Node *n = d->head;
  while (n != NULL) {
    writei(n->val, line++);
    n = n->next;
  }
  // free(n);
  // RVCMemoryPoolDeallocate(0, n);
}

// Function to return size of deque
uint32_t size(volatile Deque *d) {
  uint32_t s = 0;
  struct Node *n = d->head;
  while (n != NULL) {
    s++;
    n = n->next;
  }
  // free(n);
  // RVCMemoryPoolDeallocate(0, n);
  return s;
}

extern volatile TCBArray tcb;

void push_back_prio(volatile PrioDeque *d, TThreadID tid) {
  if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_LOW) {
    push_back(d->low, tid);
  } else if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_NORMAL) {
    push_back(d->norm, tid);
  } else if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_HIGH) {
    push_back(d->high, tid);
  }
}

TThreadID pop_front_prio(volatile PrioDeque *d) {
  TThreadID t;
  if (isEmpty(d->high) == 0) {
    t = pop_front(d->high);
  } else if (isEmpty(d->norm) == 0) {
    t = pop_front(d->norm);
  } else if (isEmpty(d->low) == 0) {
    t = pop_front(d->low);
  } else {
    t = 0;
  }
  return t;
}

void remove_prio(volatile PrioDeque *d, TThreadID tid) {
  if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_LOW) {
    removeT(d->low, tid);
  } else if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_NORMAL) {
    removeT(d->norm, tid);
  } else if (tcb.threads[tid].priority == RVCOS_THREAD_PRIORITY_HIGH) {
    removeT(d->high, tid);
  }
}

void tcb_init(volatile TCBArray *a, size_t initialSize) {
  uint32_t p = 0;
  RVCMemoryPoolCreate(a->threads, initialSize * sizeof(Thread), &p);
  RVCMemoryPoolAllocate(p, initialSize * sizeof(Thread),
                        (void **)&(a->threads));
  a->mPoolID = p;
  // a->threads = malloc(initialSize * sizeof(Thread));
  a->used = 0;
  a->size = initialSize;
}

void tcb_push_back(volatile TCBArray *a, Thread element) {
  if (a->used == a->size) {
    a->size *= 2;
    RVCMemoryPoolAllocate(a->mPoolID, a->size * sizeof(Thread),
                          (void **)&(a->threads));
    // a->threads = realloc(a->threads, a->size * sizeof(Thread));
  }
  a->threads[a->used++] = element;
}

// extern volatile allocStruct freeChunks;

void mp_init(volatile MemoryPoolArray *a) {
  a->used = 0;
  /* for (int i = 0; i < initialSize; i++)
    AllocStructDeallocate((allocStructRef)&freeChunks, (void *)&(a->chunks[i]));
  */
}

void mp_push_back(volatile MemoryPoolArray *a, SMemoryPoolFreeChunk element) {
  a->chunks[a->used++] = element;
}

void mutex_init(volatile MCBArray *a, size_t initialSize) {
  uint32_t p = 0;
  RVCMemoryPoolCreate(a->mutexes, initialSize * sizeof(Mutex), &p);
  RVCMemoryPoolAllocate(p, initialSize * sizeof(Mutex), (void **)&(a->mutexes));
  a->mPoolID = p;
  // a->mutex = malloc(initialSize * sizeof(Mutex));
  a->used = 0;
  a->size = initialSize;
}

void mutex_push_back(volatile MCBArray *a, Mutex element) {
  if (a->used == a->size) {
    a->size *= 2;
    RVCMemoryPoolAllocate(a->mPoolID, a->size * sizeof(Mutex),
                          (void **)&(a->mutexes));
    // a->mutexes = realloc(a->mutexes, a->size * sizeof(Mutex));
  }
  a->mutexes[a->used++] = element;
}

uint32_t pd_size(volatile PrioDeque *d) {
  uint32_t s = 0;
  if (d->high != NULL)
    s += size(d->high);
  if (d->norm != NULL)
    s += size(d->norm);
  if (d->low != NULL)
    s += size(d->low);
  return s;
}