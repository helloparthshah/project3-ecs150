#ifndef RVCOS_H
#define RVCOS_H

#include <stdint.h>

#define RVCOS_STATUS_FAILURE ((TStatus)0x00)
#define RVCOS_STATUS_SUCCESS ((TStatus)0x01)
#define RVCOS_STATUS_ERROR_INVALID_PARAMETER ((TStatus)0x02)
#define RVCOS_STATUS_ERROR_INVALID_ID ((TStatus)0x03)
#define RVCOS_STATUS_ERROR_INVALID_STATE ((TStatus)0x04)
#define RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES ((TStatus)0x05)

#define RVCOS_THREAD_STATE_CREATED ((TThreadState)0x01)
#define RVCOS_THREAD_STATE_DEAD ((TThreadState)0x02)
#define RVCOS_THREAD_STATE_RUNNING ((TThreadState)0x03)
#define RVCOS_THREAD_STATE_READY ((TThreadState)0x04)
#define RVCOS_THREAD_STATE_WAITING ((TThreadState)0x05)

#define RVCOS_THREAD_PRIORITY_LOW ((TThreadPriority)0x01)
#define RVCOS_THREAD_PRIORITY_NORMAL ((TThreadPriority)0x02)
#define RVCOS_THREAD_PRIORITY_HIGH ((TThreadPriority)0x03)

#define RVCOS_THREAD_ID_INVALID ((TThreadID)-1)

#define RVCOS_TIMEOUT_INFINITE ((TTick)0)
#define RVCOS_TIMEOUT_IMMEDIATE ((TTick)-1)

typedef uint32_t TStatus, *TStatusRef;
typedef uint32_t TTick, *TTickRef;
typedef int32_t TThreadReturn, *TThreadReturnRef;
typedef uint32_t TMemorySize, *TMemorySizeRef;
typedef uint32_t TThreadID, *TThreadIDRef;
typedef uint32_t TThreadPriority, *TThreadPriorityRef;
typedef uint32_t TThreadState, *TThreadStateRef;
typedef char TTextCharacter, *TTextCharacterRef;

typedef TThreadReturn (*TThreadEntry)(void *);

typedef struct {
  uint32_t DLeft : 1;
  uint32_t DUp : 1;
  uint32_t DDown : 1;
  uint32_t DRight : 1;
  uint32_t DButton1 : 1;
  uint32_t DButton2 : 1;
  uint32_t DButton3 : 1;
  uint32_t DButton4 : 1;
  uint32_t DReserved : 26;
} SControllerStatus, *SControllerStatusRef;

/* RVCInitialize() initializes the cartridges main thread and sets the
cartridges global pointer through the gp variable. This system call be invoked
as part of the cartridge startup code. */
TStatus RVCInitialize(uint32_t *gp);

TStatus RVCTickMS(uint32_t *tickmsref);
TStatus RVCTickCount(TTickRef tickref);

/* RVCThreadCreate() creates a thread in the RVCOS. Once created the thread is
 in the created state RVCOS_THREAD_STATE_CREATED.  The entryparameter
 specifies the  function  of  the thread, and paramspecifies the parameter
 that  is passed to the function. The size of the threads stack is specified
 by memsize, and the priority is specified by prio. The thread identifier is
 put into the location specified by the tidparameter. */
TStatus RVCThreadCreate(TThreadEntry entry, void *param, TMemorySize memsize,
                        TThreadPriority prio, TThreadIDRef tid);
/* RVCThreadDelete() deletes the dead thread specified by threadparameter from
 the RVCOS. */
TStatus RVCThreadDelete(TThreadID thread);
/* RVCThreadActivate() activates the newly created or dead thread specified by
 threadparameter in the RVCOS.  After  activation  the  thread  enters  the
 RVCOS_THREAD_STATE_READY state, and must begin at the entryfunction
 specified. */
TStatus RVCThreadActivate(TThreadID thread);
/* RVCThreadTerminate() terminates the thread specified by threadparameter in
 the RVCOS. After termination the thread enters the state
 RVCOS_THREAD_STATE_DEAD, and the thread return value returnval is  stored  for
 return  values  from  RVCThreadWait().  The  termination  of  a  thread can
 trigger another thread to be scheduled. */
TStatus RVCThreadTerminate(TThreadID thread, TThreadReturn returnval);
/* RVCThreadWait()  waits  for  the  thread  specified  by threadparameter  to
 terminate.  The  return value  passed  with  the  associated
 RVCThreadTerminate()  call  will  be  placed  in  the  location specified by
 returnref. RVCThreadWait() can be called multiple times per thread. */
TStatus RVCThreadWait(TThreadID thread, TThreadReturnRef returnref);
/*RVCThreadID() puts the thread identifier of the currently running thread in
 the location specified by threadref. */
TStatus RVCThreadID(TThreadIDRef threadref);
/* RVCThreadState() retrieves the state of the thread specified by threadand
 places the state in the location specified by state. */
TStatus RVCThreadState(TThreadID thread, TThreadStateRef stateref);
/* RVCThreadSleep() puts the currently running thread to sleep for tickticks. If
 tick is specified as RVCOS_TIMEOUT_IMMEDIATE the  current  process  yields
 the  remainder  of  its  processing quantum to the next ready processof equal
 priority. */
TStatus RVCThreadSleep(TTick tick);

/* RVCWriteText()  writes writesizecharacters  starting  at  the  location
 specified  by buffer to  the RISC-V Console in text mode. */
TStatus RVCWriteText(const TTextCharacter *buffer, TMemorySize writesize);
/* RVCReadController() reads the current status of the RISC-V Console controller
 into the location specified by statusref. */
TStatus RVCReadController(SControllerStatusRef statusref);

#endif
