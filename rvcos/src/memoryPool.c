#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern volatile allocStruct freeChunks;
extern volatile MemoryPoolArray InitialFreeChunks;

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
    for (int Index = 0; Index < MIN_ALLOCATION_COUNT; Index++) {
      AllocStructDeallocate((allocStructRef)&freeChunks,
                            Ptr + Index * freeChunks.DStructureSize);
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
    for (int Index = 0; Index < MIN_ALLOCATION_COUNT; Index++) {
      if (Index + 1 < MIN_ALLOCATION_COUNT) {
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
volatile int line = 0;

TStatus RVCMemoryPoolCreate(void *base, TMemorySize size,
                            TMemoryPoolIDRef memoryref) {
  if (base == NULL || size == 0 || memoryref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  // Creating the memory pool and pushing into array
  *memoryref = InitialFreeChunks.used;
  mp_push_back(&InitialFreeChunks, (SMemoryPoolFreeChunk){
                                       .DBase = base,
                                       .DSize = size,
                                       .id = InitialFreeChunks.used,
                                   });
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolDelete(TMemoryPoolID memory) {
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolQuery(TMemoryPoolID memory, TMemorySizeRef bytesleft) {
  *bytesleft = 0x40 - InitialFreeChunks.chunks[memory].DSize;
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size,
                              void **pointer) {
  if (memory >= InitialFreeChunks.used || size == 0 || pointer == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  // Allocates space using malloc
  writei(InitialFreeChunks.chunks[memory].id, line++);
  InitialFreeChunks.chunks[memory].DSize += size;
  *pointer = (void *)((uint8_t *)malloc(size));
  // *pointer = (void *)AllocateFreeChunk();
  return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolDeallocate(TMemoryPoolID memory, void *pointer) {
  // frees the memory
  free(pointer);
  // AllocStructDeallocate((allocStructRef)&freeChunks, pointer);
  return RVCOS_STATUS_SUCCESS;
}