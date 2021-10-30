// #include "RVCOS.h"
#include "Deque.h"
#include "RVCOS.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTROLLER (*((volatile uint32_t *)0x40000018))

volatile char *VIDEO_MEMORY = (volatile char *)(0x50000000 + 0xFE800);
#define CARTRIDGE (*((volatile uint32_t *)0x4000001C))

void write(const TTextCharacter *c, uint32_t line) {
  for (uint32_t i = 0; i < strlen(c); i++) {
    VIDEO_MEMORY[line * 0x40 + i] = c[i];
  }
}

void writei(uint32_t c, int line) {
  char hex[32];
  sprintf(hex, "%x", c);
  write(hex, line);
}

__attribute__((always_inline)) inline void set_tp(uint32_t tp) {
  asm volatile("lw tp, 0(%0)" ::"r"(&tp));
}

__attribute__((always_inline)) inline uint32_t get_tp(void) {
  uint32_t result;
  asm volatile("mv %0,tp" : "=r"(result));
  return result;
}

// uint32_t get_tp();

__attribute__((always_inline)) inline void set_ra(uint32_t tp) {
  asm volatile("lw ra, 0(%0)" ::"r"(tp));
}

extern void csr_write_mie(uint32_t);

extern void csr_enable_interrupts(void);

extern void csr_disable_interrupts(void);

TThreadReturn idleThread(void *param) {
  csr_enable_interrupts();
  csr_write_mie(0x888);
  while (1)
    ;
  return 0;
}

void switch_context(uint32_t **oldctx, uint32_t *newctx);

uint32_t call_on_other_gp(volatile void *param, volatile TThreadEntry entry,
                          uint32_t gp);

uint32_t *initialize_stack(uint32_t *sp, TThreadEntry fun, void *param,
                           uint32_t tp) {
  sp--;
  *sp = (uint32_t)fun; // sw      ra,48(sp)
  sp--;
  *sp = tp; // sw      tp,44(sp)
  sp--;
  *sp = 0; // sw      t0,40(sp)
  sp--;
  *sp = 0; // sw      t1,36(sp)
  sp--;
  *sp = 0; // sw      t2,32(sp)
  sp--;
  *sp = 0; // sw      s0,28(sp)
  sp--;
  *sp = 0; // sw      s1,24(sp)
  sp--;
  *sp = (uint32_t)param; // sw      a0,20(sp)
  sp--;
  *sp = 0; // sw      a1,16(sp)
  sp--;
  *sp = 0; // sw      a2,12(sp)
  sp--;
  *sp = 0; // sw      a3,8(sp)
  sp--;
  *sp = 0; // sw      a4,4(sp)
  sp--;
  *sp = 0; // sw      a5,0(sp)
  return sp;
}

volatile Deque *high;
volatile Deque *norm;
volatile Deque *low;
volatile Thread tcb[256];
volatile int id_count = 1;
volatile int curr_running = 1;
volatile uint32_t ticks = 0;
volatile uint32_t cart_gp;

void push_back_prio(TThreadID tid) {
  if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_LOW) {
    push_back(low, tid);
  } else if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_NORMAL) {
    push_back(norm, tid);
  } else if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_HIGH) {
    push_back(high, tid);
  }
}

TThreadID pop_front_prio() {
  TThreadID t;
  if (isEmpty(high) == 0) {
    t = pop_front(high);
  } else if (isEmpty(norm) == 0) {
    t = pop_front(norm);
  } else if (isEmpty(low) == 0) {
    t = pop_front(low);
  } else {
    t = 0;
  }
  return t;
}

void remove_prio(TThreadID tid) {
  if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_LOW) {
    removeT(low, tid);
  } else if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_NORMAL) {
    removeT(norm, tid);
  } else if (tcb[tid].priority == RVCOS_THREAD_PRIORITY_HIGH) {
    removeT(high, tid);
  }
}


void dec_tick() {
  // Looping through the threads to check which are sleeping
  for (int i = 1; i < id_count; i++) {
    if (tcb[i].is_sleeping == 1) {
      // If thread has finished sleeping
      if (tcb[i].sleep_for == 0) {
        // Stop sleeping and add back to queue
        tcb[i].is_sleeping = 0;
        tcb[i].state = RVCOS_THREAD_STATE_READY;
        push_back_prio(i);
      }
      // Decrement sleep for
      tcb[i].sleep_for--;
    }
  }
}

void scheduler() {
  int old_running = curr_running;

  dec_tick();
  // Add back only if it was running previously
  if (tcb[old_running].state == RVCOS_THREAD_STATE_RUNNING) {
    tcb[old_running].state = RVCOS_THREAD_STATE_READY;

    push_back_prio(old_running);
  }

  // Getting the highest priority thread
  curr_running = pop_front_prio();

  // Switching the context
  tcb[curr_running].state = RVCOS_THREAD_STATE_RUNNING;
  if (old_running != curr_running)
    switch_context((uint32_t **)&tcb[old_running].ctx, tcb[curr_running].ctx);
}

TThreadReturn skeleton(void *param) {
  // Setting the thread pointer to the current running
  set_tp(curr_running);
  // Enabling interrupts
  csr_enable_interrupts();
  csr_write_mie(0x888);
  // Calling the entry function and storing the return value
  uint32_t ret_value = call_on_other_gp(tcb[curr_running].param,
                                        tcb[curr_running].entry, cart_gp);
  // Disabling the interrupts
  csr_write_mie(0x0);
  csr_disable_interrupts();
  // Calling terminate
  RVCThreadTerminate(curr_running, ret_value);
  return 0;
}

TStatus RVCInitialize(uint32_t *gp) {
  if (gp == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Allocating memory to the priority deques
  high = dmalloc();
  norm = dmalloc();
  low = dmalloc();
  if (high == NULL || norm == NULL || low == NULL)
    return RVCOS_STATUS_FAILURE;
  // Storing the cartridge gp
  cart_gp = (uint32_t)gp;
  // Create idle thread
  tcb[0].ctx = initialize_stack(malloc(1024) + 1024, idleThread, 0, 0);
  tcb[0].entry = idleThread;
  tcb[0].id = 0;
  tcb[0].priority = 0;
  tcb[0].memsize = 1024;
  tcb[0].is_sleeping = 0;
  tcb[0].state = RVCOS_THREAD_STATE_READY;
  // Create main thread
  tcb[1].id = 1;
  tcb[1].priority = RVCOS_THREAD_PRIORITY_NORMAL;
  tcb[1].state = RVCOS_THREAD_STATE_RUNNING;
  tcb[1].is_sleeping = 0;
  // Make tp point to main
  curr_running = 1;
  set_tp(1);
  id_count++;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCTickMS(uint32_t *tickmsref) {
  // Returnign the current ticksms
  if (tickmsref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  *tickmsref = 2;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCTickCount(TTickRef tickref) {
  if (tickref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Returning the current tick count
  *tickref = ticks;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadCreate(TThreadEntry entry, void *param, TMemorySize memsize,
                        TThreadPriority prio, TThreadIDRef tid) {
  if (entry == NULL || tid == NULL || memsize == 0 || prio > 3)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Creating the thread and adding to the tcb
  tcb[id_count].entry = entry;
  tcb[id_count].id = id_count;
  *tid = id_count;
  tcb[id_count].priority = prio;
  tcb[id_count].param = param;
  tcb[id_count].memsize = memsize;
  tcb[id_count].state = RVCOS_THREAD_STATE_CREATED;
  tcb[id_count].is_sleeping = 0;
  id_count++;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadDelete(TThreadID thread) {
  if (tcb[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  Thread t;
  free(tcb[thread].ctx);
  tcb[thread] = t;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadActivate(TThreadID thread) {
  if (tcb[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  // Initializing context
  tcb[thread].ctx =
      initialize_stack(malloc(tcb[thread].memsize) + tcb[thread].memsize,
                       skeleton, tcb[thread].param, id_count);
  if (tcb[thread].ctx == NULL)
    return RVCOS_STATUS_FAILURE;
  //  Setting the state to ready
  tcb[thread].state = RVCOS_THREAD_STATE_READY;
  // Pushing the the priority deque
  push_back_prio(thread);
  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadTerminate(TThreadID thread, TThreadReturn returnval) {
  if (tcb[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  // Setting state to dead
  tcb[thread].state = RVCOS_THREAD_STATE_DEAD;
  // Setting the return value
  tcb[thread].return_val = returnval;
  // If current is waited by then set all to ready
  if (tcb[thread].waited_by != NULL)
    while (isEmpty(tcb[thread].waited_by) == 0) {
      uint32_t wid = pop_front(tcb[thread].waited_by);
      tcb[wid].state = RVCOS_THREAD_STATE_READY;
      push_back_prio(wid);
    }

  // Removing the thread from deque
  remove_prio(thread);
  
  scheduler();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadWait(TThreadID thread, TThreadReturnRef returnref,
                      TTick timeout) {
  if (tcb[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (returnref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  TThreadID wid = curr_running;
  // Setting state to waiting
  tcb[wid].state = RVCOS_THREAD_STATE_WAITING;

  // Remove from deque
  remove_prio(wid);

  // Adding to the waiting queue
  if (tcb[thread].waited_by == NULL)
    tcb[thread].waited_by = dmalloc();
  if (tcb[thread].waited_by == NULL)
    return RVCOS_STATUS_FAILURE;
  push_back(tcb[thread].waited_by, wid);

  // Scheduling till it's dead
  while (tcb[thread].state != RVCOS_THREAD_STATE_DEAD) {
    scheduler();
  }

  // Setting the return value
  *returnref = tcb[thread].return_val;
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadSleep(TTick tick) {
  if (tick == RVCOS_TIMEOUT_INFINITE)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;

  if (tick == RVCOS_TIMEOUT_IMMEDIATE) {
    scheduler();
  } else {
    TThreadID sid = get_tp();
    // Setting state to waiting
    tcb[sid].state = RVCOS_THREAD_STATE_WAITING;
    tcb[sid].sleep_for = tick;
    tcb[sid].is_sleeping = 1;
    // Removing from the deque
    remove_prio(sid);
        // Calling scheduler
    scheduler();
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadID(TThreadIDRef threadref) {
  // Setting the tp
  *threadref = get_tp();
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadState(TThreadID thread, TThreadStateRef stateref) {
  if (tcb[thread].id != thread)
    return RVCOS_STATUS_ERROR_INVALID_ID;
  if (stateref == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // returning the state
  *stateref = tcb[thread].state;
  return RVCOS_STATUS_SUCCESS;
}

volatile int cursor = 0;
TStatus RVCWriteText(const TTextCharacter *buffer, TMemorySize writesize) {
  if (buffer == NULL)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Looping through the buffer
  for (uint32_t i = 0; i < writesize; i++) {
    // if backspace move cursor back
    if (buffer[i] == '\b') {
      if (cursor > 0)
        VIDEO_MEMORY[--cursor] = ' ';
    } else if (buffer[i] == '\n') {
      // Jump to nextline
      cursor += 0x40;
      // Move back to start of the line
      cursor -= cursor % 0x40;
    } else {
      // else printing the charactor
      VIDEO_MEMORY[cursor++] = buffer[i];
    }
    if ((cursor) / 0x40 >= 36) {
      // Shifting everything up when reach end of screen
      memcpy((void *)VIDEO_MEMORY, (void *)VIDEO_MEMORY + 0x40, 0x40 * 36);
      cursor -= 0x40;
    }
  }
  return RVCOS_STATUS_SUCCESS;
}

TStatus RVCReadController(SControllerStatusRef statusref) {
  if (!statusref)
    return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
  // Setting the read controller
  *(uint32_t *)statusref = CONTROLLER;
  return RVCOS_STATUS_SUCCESS;
}

void enter_cartridge();

// Checking if cartridge is inserted
volatile int isInit = 0;

volatile uint32_t *saved_sp;

int main() {
  while (1) {
    if (CARTRIDGE & 0x1 && isInit == 0) {
      isInit = 1;
      enter_cartridge();
    }
    if (!(CARTRIDGE & 0x1) && isInit == 1) {
      ticks = 0;
      /* cursor = 0;
      for (int i = 0; i < 36 * 64; i++) {
        VIDEO_MEMORY[i] = ' ';
      } */
      isInit = 0;
    }
  }
  return 0;
}

uint32_t c_syscall_handler(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
                           uint32_t a4, uint32_t code) {
  void *p0 = (void *)a0;
  void *p1 = (void *)a1;
  void *p2 = (void *)a2;
  void *p4 = (void *)a4;
  // Calling syscalls based on ecall code
  if (code == 0) {
    return RVCInitialize(p0);
  } else if (code == 1) {
    return RVCThreadCreate((TThreadEntry)p0, p1, (TMemorySize)a2, a3, p4);
  } else if (code == 2) {
    return RVCThreadDelete(a0);
  } else if (code == 3) {
    return RVCThreadActivate(a0);
  } else if (code == 4) {
    return RVCThreadTerminate(a0, a1);
  } else if (code == 5) {
    return RVCThreadWait(a0, (TThreadReturnRef)p1, a2);
  } else if (code == 6) {
    return RVCThreadID(p0);
  } else if (code == 7) {
    return RVCThreadState(a0, p1);
  } else if (code == 8) {
    return RVCThreadSleep(a0);
  } else if (code == 9) {
    return RVCTickMS(p0);
  } else if (code == 10) {
    return RVCTickCount(p0);
  } else if (code == 11) {
    return RVCWriteText(p0, a1);
  } else if (code == 12) {
    return RVCReadController((SControllerStatusRef)p0);
  } else if (code == 13) {
    return RVCMemoryPoolCreate(p0, a1, a2);
  } else if (code == 14) {
    return RVCMemoryPoolDelete(a0);
  } else if (code == 15) {
    return RVCMemoryPoolQuery(a0, p1);
  } else if (code == 16) {
    return RVCMemoryPoolAllocate(a0, p1, p2);
  } else if (code == 17) {
    return RVCMemoryPoolDeallocate(a0, p1);
  } else if (code == 18) {
    return RVCMutexCreate(p0);
  } else if (code == 19) {
    return RVCMutexDelete(a0);
  } else if (code == 20) {
    return RVCMutexQuery(a0, p1);
  } else if (code == 21) {
    return RVCMutexAcquire(a0, p1);
  } else if (code == 22) {
    return RVCMutexRelease(a0);
  }
  return RVCOS_STATUS_FAILURE;
}
