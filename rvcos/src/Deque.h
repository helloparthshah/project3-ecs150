#ifndef DEQUE
#define DEQUE
#include "RVCOS.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
  struct Node *next;
  struct Node *prev;
  TThreadID val;
};

typedef struct {
  struct Node *head;
  struct Node *tail;
} Deque;

typedef struct {
  Deque *high;
  Deque *norm;
  Deque *low;
} PrioDeque;

typedef struct {
  uint32_t *ctx;
  TThreadEntry entry;
  void *param;
  TThreadID id;
  TMemorySize memsize;
  TThreadPriority priority;
  TThreadState state;
  Deque *waited_by;
  uint32_t return_val;
  int sleep_for;
  int wait_timeout;
  int mutex_timeout;
  TMutexID waiting_for_mutex;
  Deque *mutex_id;
} Thread;

typedef struct {
  TMutexID id;
  TThreadID owner;
  TMutexState state;
  PrioDeque *waiting;
} Mutex;

typedef struct {
  Thread *threads;
  size_t used;
  size_t size;
} TCBArray;

typedef struct {
  Mutex *mutexes;
  size_t used;
  size_t size;
} MutexArray;

void tcb_init(volatile TCBArray *a, size_t initialSize);
void tcb_push_back(volatile TCBArray *a, Thread element);

void mutex_init(volatile MutexArray *a, size_t initialSize);
void mutex_push_back(volatile MutexArray *a, Mutex element);

Deque *dmalloc();
PrioDeque *pdmalloc();

void print(volatile Deque *d, uint32_t line);
uint32_t size(volatile Deque *d);
void push_front(volatile Deque *d, TThreadID v);
void push_back(volatile Deque *d, TThreadID v);

void removeT(volatile Deque *d, TThreadID v);

int isEmpty(volatile Deque *d);

TThreadID pop_front(volatile Deque *d);
TThreadID pop_back(volatile Deque *d);

TThreadID front(volatile Deque *d);
TThreadID end(volatile Deque *d);

void push_back_prio(volatile PrioDeque *d, TThreadID tid);
TThreadID pop_front_prio(volatile PrioDeque *d);
void remove_prio(volatile PrioDeque *d, TThreadID tid);
uint32_t pd_size(volatile PrioDeque *d);

#endif