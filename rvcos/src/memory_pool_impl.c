#include "RVCOS.h"
// #include "Deque.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_CHUNK_SIZE 0x40

// get a system pool pointer (for use as the start point memory pool)
// use malloc for this (for now) - but this will be replaced with a system
// global pointer

// structure to keep track of the system memory pool
typedef struct {
  uint8_t *base_ptr; // base pointer system pool
                     // number of chunks remaining the system pool
  uint32_t chunks_remaining;
  // number of chunks allocated from the system pool
  uint32_t chunks_allocated;
} SSystemPool, *SSystemPoolRef;

SSystemPoolRef system_pool = NULL;
// structure to keep track of the memory pool
typedef struct {
  uint8_t id;
  uint8_t *base_ptr; // base pointer to memory pool
                     // number of chunks remaining the memory pool
  uint32_t chunks_remaining;
  // number of chunks allocated from the memory pool
  uint32_t chunks_allocated;
} SMemoryPool, *SMemoryPoolRef;

typedef struct {
  SMemoryPool *memory_pools;
  int used;
  int size;
} MemoryPoolArray, *MemoryPoolArrayRef;

void mp_push_back(volatile MemoryPoolArray *a, SMemoryPool element) {
  if (a->used == a->size) {
    a->size *= 2;
    SMemoryPool *t = a->memory_pools;
    RVCMemoryPoolAllocate(0, a->size * sizeof(SMemoryPool),
                          (void **)&(a->memory_pools));
    for (int i = 0; i < a->used; i++) {
      a->memory_pools[i] = t[i];
    }
  }
  a->memory_pools[a->used++] = element;
}

volatile MemoryPoolArrayRef memory_pool_array;

#define RVCMemoryAllocate(size, pointer)                                       \
  RVCMemoryPoolAllocate(RVCOS_MEMORY_POOL_ID_SYSTEM, (size), (pointer))
#define RVCMemoryDeallocate(pointer)                                           \
  RVCMemoryPoolDeallocate(RVCOS_MEMORY_POOL_ID_SYSTEM, (pointer))

// this is the system memory pool init
// this is called by the RVCInit function
TStatus RVCSystemPoolInit() {
  //   RVCMemoryAllocate(sizeof(MemoryPoolArray), (void **)&memory_pool_array);
  memory_pool_array->used = 0;
  memory_pool_array->size = 1;

  system_pool->base_ptr = malloc(1024); // change to heap base
}

// create memory pool at base
TStatus RVCMemoryPoolCreate(void *base, TMemorySize size,
                            TMemoryPoolIDRef memoryref) {
  if (size < 2 * MIN_CHUNK_SIZE || base == NULL || memoryref == NULL) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  SMemoryPool *mem_pool;
  mem_pool->id = memory_pool_array->used;
  memoryref = mem_pool->id;
  mem_pool->base_ptr = base;
  mem_pool->chunks_remaining =
      (size / MIN_CHUNK_SIZE) + (size % MIN_CHUNK_SIZE);
  mem_pool->chunks_allocated = 0;
  mp_push_back(memory_pool_array, *mem_pool);
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDelete(TMemoryPoolID memory) {
  if (memory >= memory_pool_array->used) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  SMemoryPoolRef mem_pool = &memory_pool_array->memory_pools[memory];
  if (mem_pool->chunks_allocated != 0) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  mem_pool->base_ptr = NULL;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolQuery(TMemoryPoolID memory, TMemorySizeRef bytesleft) {
  if (memory >= memory_pool_array->used) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  SMemoryPoolRef mem_pool = &memory_pool_array->memory_pools[memory];
  *bytesleft = mem_pool->chunks_remaining * MIN_CHUNK_SIZE;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size,
                              void **pointer) {}
TStatus RVCMemoryPoolDeallocate(TMemoryPoolID memory, void *pointer);

int main() {
  //
  return 0;
}