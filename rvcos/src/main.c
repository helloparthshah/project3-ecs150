#include "Deque.h"
#include "RVCOS.h"

#define CARTRIDGE (*((volatile uint32_t *)0x4000001C))
#define MTIME_LOW (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH (*((volatile uint32_t *)0x40000014))

void enter_cartridge();
extern void csr_write_mie(uint32_t);

extern void csr_enable_interrupts(void);

extern void csr_disable_interrupts(void);
// Checking if cartridge is inserted
volatile int isInit = 0;

volatile uint32_t *saved_sp;

int main() {
  /* char *c = "\x1B[30;40HHello World!";
  RVCWriteText(c, strlen(c)); */
  while (1) {
    if (CARTRIDGE & 0x1 && isInit == 0) {
      isInit = 1;

      /* uint64_t NewCompare = (((uint64_t)MTIME_HIGH) << 32) | MTIME_LOW;
      NewCompare += RVCOS_TICKS_MS;
      MTIMECMP_HIGH = NewCompare >> 32;
      MTIMECMP_LOW = NewCompare;
      csr_enable_interrupts();
      csr_write_mie(0x888); */

      enter_cartridge();
    }
    if (!(CARTRIDGE & 0x1) && isInit == 1) {
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

  TStatus status = RVCOS_STATUS_FAILURE;
  // Calling syscalls based on ecall code
  if (code == 0) {
    status = RVCInitialize(p0);
  } else if (code == 1) {
    status = RVCThreadCreate((TThreadEntry)p0, p1, (TMemorySize)a2, a3, p4);
  } else if (code == 2) {
    status = RVCThreadDelete(a0);
  } else if (code == 3) {
    status = RVCThreadActivate(a0);
  } else if (code == 4) {
    status = RVCThreadTerminate(a0, a1);
  } else if (code == 5) {
    status = RVCThreadWait(a0, (TThreadReturnRef)p1, a2);
  } else if (code == 6) {
    status = RVCThreadID(p0);
  } else if (code == 7) {
    status = RVCThreadState(a0, p1);
  } else if (code == 8) {
    status = RVCThreadSleep(a0);
  } else if (code == 9) {
    status = RVCTickMS(p0);
  } else if (code == 10) {
    status = RVCTickCount(p0);
  } else if (code == 11) {
    status = RVCWriteText(p0, a1);
  } else if (code == 12) {
    status = RVCReadController((SControllerStatusRef)p0);
  } else if (code == 13) {
    status = RVCMemoryPoolCreate(p0, a1, p2);
  } else if (code == 14) {
    status = RVCMemoryPoolDelete(a0);
  } else if (code == 15) {
    status = RVCMemoryPoolQuery(a0, p1);
  } else if (code == 16) {
    status = RVCMemoryPoolAllocate(a0, a1, p2);
  } else if (code == 17) {
    status = RVCMemoryPoolDeallocate(a0, p1);
  } else if (code == 18) {
    status = RVCMutexCreate(p0);
  } else if (code == 19) {
    status = RVCMutexDelete(a0);
  } else if (code == 20) {
    status = RVCMutexQuery(a0, p1);
  } else if (code == 21) {
    status = RVCMutexAcquire(a0, a1);
  } else if (code == 22) {
    status = RVCMutexRelease(a0);
  }
  return status;
}
