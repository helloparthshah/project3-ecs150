#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern volatile MemoryPoolArray memory_pool_array;

typedef struct freeNodeStruct freeNode, *freeNodeRef;

struct freeNodeStruct {
  struct freeNodeStruct *next, *prev;
};

freeNodeRef freeNodesList;
freeNodeRef allocNodesList;

extern uint8_t _pool_size;

void initSystemPool(void *ptr) {
  ptr = malloc(10240);
  if (ptr != NULL) {
    for (unsigned long i = 0; i < _pool_size / MIN_ALLOCATION_COUNT; i++) {
      freeNodeRef pCurUnit = (freeNodeRef)((char *)ptr + i * sizeof(freeNode));

      pCurUnit->prev = NULL;
      pCurUnit->next = freeNodesList; // Insert the new unit at head.

      if (freeNodesList != NULL) {
        freeNodesList->prev = pCurUnit;
      }
      freeNodesList = pCurUnit;
    }
  }
}

void *Alloc() {
  freeNodeRef pCurUnit = freeNodesList;
  freeNodesList = pCurUnit->next; // Get a unit from free linkedlist.
  if (NULL != freeNodesList) {
    freeNodesList->prev = NULL;
  }

  pCurUnit->next = allocNodesList;

  if (NULL != allocNodesList) {
    allocNodesList->prev = pCurUnit;
  }
  allocNodesList = pCurUnit;

  return (void *)((char *)pCurUnit + sizeof(freeNode));
}

void write(char *, uint32_t);

TStatus RVCMemoryPoolCreate(void *base, TMemorySize size,
                            TMemoryPoolIDRef memoryref) {
  if (base == NULL || size == 0 || memoryref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  // Creating the memory pool and pushing into array
  *memoryref = memory_pool_array.used;
  mp_push_back(&memory_pool_array, (SMemoryPoolFreeChunk){
                                       .DBase = base,
                                       .DSize = size,
                                       .id = memory_pool_array.used,
                                   });
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDelete(TMemoryPoolID memory) {
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolQuery(TMemoryPoolID memory, TMemorySizeRef bytesleft) {
  if (memory >= memory_pool_array.used || bytesleft == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  *bytesleft = MIN_ALLOCATION_COUNT - memory_pool_array.chunks[memory].DSize;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size,
                              void **pointer) {
  if (memory >= memory_pool_array.used || size == 0 || pointer == NULL) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  if (memory == RVCOS_MEMORY_POOL_ID_SYSTEM)
    *pointer = Alloc();
  /* if (size >= MIN_ALLOCATION_COUNT * freeChunks.DStructureSize -
                  memory_pool_array.chunks[memory].DSize)
    return RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES; */

  // Allocates space using malloc
  // writei(freeChunks.DCount, 20);
  /* *pointer = (void *)((uint8_t *)memory_pool_array.chunks[memory].DBase +
                      memory_pool_array.chunks[memory].DSize); */

  *pointer = (void *)((uint8_t *)malloc(size));
  // *pointer = Alloc();
  // memory_pool_array.chunks[memory].DSize += size;
  // Check if we need to allocate a new chunk if crosses the multiple of min
  // size

  /* if ((memory_pool_array.chunks[memory].DSize - size) % MIN_ALLOCATION_COUNT
  != memory_pool_array.chunks[memory].DSize % MIN_ALLOCATION_COUNT) { *pointer =
  (void *)((uint8_t *)memory_pool_array.chunks[memory].DBase +
                        memory_pool_array.chunks[memory].DSize - size);
  } else {
    *pointer = (void *)AllocateFreeChunk();
  } */
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDeallocate(TMemoryPoolID memory, void *pointer) {
  // frees the memory
  free(pointer);
  // AllocStructDeallocate((allocStructRef)&freeChunks, pointer);
  return RVCOS_STATUS_SUCCESS;
}