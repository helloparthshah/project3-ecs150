#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern volatile allocStruct freeChunks;
extern volatile MemoryPoolArray memory_pool_array;

void AllocStructInit(volatile allocStructRef alloc, TMemorySize size) {
  alloc->DCount = 0;
  alloc->DStructureSize = size;
  alloc->DFirstFree = NULL;
}

void *MemoryAlloc(int size) {
  AllocateFreeChunk();
  return malloc(size);
}

volatile int SuspendAllocationOfFreeChunks = 0;

SMemoryPoolFreeChunkRef AllocateFreeChunk(void) {
  if (3 > freeChunks.DCount && !SuspendAllocationOfFreeChunks) {
    SuspendAllocationOfFreeChunks = 1;
    uint8_t *Ptr =
        MemoryAlloc(freeChunks.DStructureSize * MIN_ALLOCATION_COUNT);
    for (int i = 0; i < MIN_ALLOCATION_COUNT; i++) {
      AllocStructDeallocate((allocStructRef)&freeChunks,
                            Ptr + i * freeChunks.DStructureSize);
    }
    SuspendAllocationOfFreeChunks = 0;
  }
  return (SMemoryPoolFreeChunkRef)AllocStructAllocate(
      (allocStructRef)&freeChunks);
}

void *AllocStructAllocate(allocStructRef alloc) {
  if (!alloc->DCount) {
    alloc->DFirstFree =
        MemoryAlloc(alloc->DStructureSize * MIN_ALLOCATION_COUNT);
    freeNodeRef Current = alloc->DFirstFree;
    for (int i = 0; i < MIN_ALLOCATION_COUNT; i++) {
      if (i + 1 < MIN_ALLOCATION_COUNT) {
        Current->DNext =
            (freeNodeRef)((uint8_t *)Current + alloc->DStructureSize);
        Current = Current->DNext;
      } else {
        Current->DNext = NULL;
      }
    }
    alloc->DCount = MIN_ALLOCATION_COUNT;
  }
  freeNodeRef NewStruct = alloc->DFirstFree;
  alloc->DFirstFree = alloc->DFirstFree->DNext;
  alloc->DCount--;
  return NewStruct;
}

void AllocStructDeallocate(volatile allocStructRef alloc, void *obj) {
  freeNodeRef OldStruct = (freeNodeRef)obj;
  alloc->DCount++;
  OldStruct->DNext = alloc->DFirstFree;
  alloc->DFirstFree = OldStruct;
}

void writei(uint32_t, uint32_t);
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
  *bytesleft = 0x40 - memory_pool_array.chunks[memory].DSize;
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size,
                              void **pointer) {
  if (memory >= memory_pool_array.used || size == 0 || pointer == NULL) {
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  }
  if (memory_pool_array.chunks[memory].DSize + size >
      MAX_POOLS * MIN_ALLOCATION_COUNT * freeChunks.DStructureSize)
    return RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES;

  // Allocates space using malloc
  // writei(freeChunks.DCount, 20);
  /* *pointer = (void *)((uint8_t *)memory_pool_array.chunks[memory].DBase +
                      memory_pool_array.chunks[memory].DSize); */

  memory_pool_array.chunks[memory].DSize += size;
  *pointer = (void *)((uint8_t *)malloc(size));
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