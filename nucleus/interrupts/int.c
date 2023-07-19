//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel
/****************
This module handles the interrupts

****************/

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/util.h"

#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/trap.e"
#include "../../h/main.e"


state_t* oldClock;
state_t* oldFloppy;
state_t* oldDisk;
state_t* oldPrint;
state_t* oldTerm;

struct devSemStruct{
  devreg_t* devReg;
  int sem;
  int status;
  int dadd;
}devStruct[15];

long second = SECOND;

int pseudoClock = 0;//SEMAPHORE; NOT A REAL CLOCK!!
int timeslice = (SECOND / 1000) * 5; // 5 ms timeslice
int intervalsLeft;

//static void waitforplock();
//static void waitforio();
void intdeadlock();
void intschedule();

void static intterminalhandler();
void static intprinterhandler();
void static intdiskhandler();
void static intfloppyhandler();
void static intclockhandler();

void static inthandler();
void static intsemop();
void static sleep();


/*
Initializes, loads EVT entries, sets new interrupt areas, etc
*/
intinit()
{
  
  //init devSemStruct
  int i = 0;
  for(i = 0; i < 15; i++)
  {
    int offset = 0x10 * i;
    
    devStruct[i].devReg = (devreg_t*)(0x1400+offset);
    devStruct[i].sem = 0;
    devStruct[i].status = ENULL;
    devStruct[i].dadd = ENULL;
  }
  //initialize the various trap areas
  *(int*)0x100 = (int)STLDTERM0;			/*   0x100            interrupt */
  *(int*)0x104 = (int)STLDTERM1;			/*   0x104            interrupt */
  *(int*)0x108 = (int)STLDTERM2;			/*   0x108            interrupt */
  *(int*)0x10c = (int)STLDTERM3;			/*   0x10c            interrupt */
  *(int*)0x110 = (int)STLDTERM4;			/*   0x110            interrupt */
  *(int*)0x114 = (int)STLDPRINT0;		/*   0x114            interrupt */
  *(int*)0x11c = (int)STLDDISK0;			/*   0x11c            interrupt */
  *(int*)0x12c = (int)STLDFLOPPY0;		/*   0x12c            interrupt */
  *(int*)0x140 = (int)STLDCLOCK;			/*   0x140            interrupt */
  
  intervalsLeft = (SECOND / 10)/timeslice; //100 ms broken up into 100 / timeslice intervals
  
  //init old areas
  oldTerm = (state_t*)0x9c8;
  oldPrint = (state_t*)0xa60;
  oldDisk = (state_t*)0xaf8;
  oldFloppy = (state_t*)0xb90;
  oldClock = (state_t*)0xc28;
  
  //set new areas for interrupts
  state_t* newTerm = (oldTerm + 1);
  state_t* newPrint = (oldPrint + 1);
  state_t* newDisk = (oldDisk +1);
  state_t* newFloppy = (oldFloppy + 1);
  state_t* newClock = (oldClock + 1);
  
  //store
  //load new areas with PC of high level handlers
  //store
  //load new areas with PC of high level handlers
  STST(newTerm);    
  newTerm->s_pc = (int)intterminalhandler;
  STST(newPrint);    
  newPrint->s_pc = (int)intprinterhandler;
  STST(newDisk);    
  newDisk->s_pc = (int)intdiskhandler;
  STST(newFloppy);    
  newFloppy->s_pc = (int)intfloppyhandler;
  STST(newClock);    
  newClock->s_pc = (int)intclockhandler;

}

/*
does intsemop on pseudoClock
*/
waitforpclock(old)
state_t *old;
{
  (headQueue(RQ))->p_s = *old;
  intsemop(&pseudoClock, LOCK);
}


/*
This func checks if interrupt already occurred. If it has, decrement the semaphore and pass completion
status to process. Otherwise, do intsemop(LOCK) on semaphore corresponding to each device.
*/
waitforio(old)
state_t *old;
{

  int devNum = old->s_r[4];
  int currSem = devStruct[devNum].sem;
  int* currSemAddr = &currSem;
  proc_t* p = headQueue(RQ);

  if(currSem > 0)
  {
    //if interrupt already occurred

    //decrement semaphore
    currSem--;
    devStruct[devNum].sem = currSem;

    //pass completion status
    old->s_r[3] = devStruct[devNum].status;
    
    //need to pass device's length register???
  }
  else if(currSem <= 0)
  {
    proc_t* proc;
    //interrupt not already occurred
      //do LOCK on sempahore
		(headQueue(RQ))->p_s = *old;
		intsemop(&devStruct[devNum].sem, LOCK);
	}

}

int checkIfEmpty()
{
  int i = 0;
  for(i = 0; i < 15; i++)
  {
    int currSem = devStruct[i].sem;
    if(headBlocked(&devStruct[i].sem) != (proc_t*) ENULL)
    {
      return FALSE;
    }
  }
  
  return TRUE;
}

/*
called when RQ is empty. Depending on where processes are blocked, schedules,
sleeps, or terminates.
*/
void intdeadlock()
{

  devreg_t* printer = (devreg_t *)0x1450;
  
  if (headBlocked(&pseudoClock) != (proc_t*) ENULL)
  {
    //if processes blocked on pseudoclock
    intschedule();
    sleep();
  }
  else
  {
    //no processes blocked on pseudoclock
    
    int isEmpty = TRUE;
    int activeSems;
    
    //check if io semaphores are empty
    isEmpty = checkIfEmpty();
    
    //check if asl empty
    activeSems = headASL();
    
    if(isEmpty == FALSE)
    {
      //if processes on I/O semaphores
      sleep();
    }
    else if(activeSems == FALSE)
    {
      //if no processes left
      /* set up printer's device registers */
      char shutdown[] = "system terminated normally";
      
      printer->d_stat = ENULL;
	    printer->d_badd = shutdown;
      int len = sizeof(shutdown);
      printer->d_amnt = len;
	    int op = IOWRITE;
	    printer->d_op = op;		

      //watch status register to see when finished
      while(TRUE)
      {
        if (devStruct[PRINT0].devReg->d_stat == NORMAL)
        {
          break;
        }
      }
      HALT();
    }
    else
    {
      //otherwise, deadlocked
      
      /* set up printer's device registers */
      
      char deadlock[] = "system deadlocked!";
      
      printer->d_stat = ENULL;
      printer->d_badd = deadlock;
      int len = sizeof(deadlock);
      printer->d_amnt = len;
	    int op = IOWRITE;
	    printer->d_op = op;
         
      
      //watch status register to see when finished
      while(TRUE)
      {
        if (devStruct[PRINT0].devReg->d_stat == NORMAL)
        {
          break;
        } 
      }
      HALT();
    }
    
    
  }
}
  

/*
Loadsd timeslice into interval timer
*/
void intschedule()
{
  LDIT(&timeslice);
}

//loads state or schedules
void auxloader(proc_t* p, state_t* oldst)
{
  if(p != (proc_t*)ENULL)
  {
    LDST(oldst);
  }
  else if(p == (proc_t*)ENULL)
  {
    schedule();
  }
}

/* ||
   VV The below functions pass device type and number to inthandler()
*/
void static intterminalhandler()
{
  proc_t* p = headQueue(RQ);
  int devNum = oldTerm->s_tmp.tmp_int.in_dno; //get dev num
  inthandler(TERMINAL, devNum);
  auxloader(p, oldTerm);
}
void static  intprinterhandler()
{
  proc_t* p = headQueue(RQ);
  int devNum = oldPrint->s_tmp.tmp_int.in_dno; //get dev num
  inthandler(PRINTER, devNum);
  auxloader(p, oldPrint);
}
void static  intdiskhandler()
{
  proc_t* p = headQueue(RQ);
  int devNum = oldDisk->s_tmp.tmp_int.in_dno; //get dev num
  inthandler(DISK, devNum);
  auxloader(p, oldDisk);
}
void static  intfloppyhandler()
{
  proc_t* p = headQueue(RQ);
  int devNum = oldFloppy->s_tmp.tmp_int.in_dno; //get dev num
  inthandler(FLOPPY, devNum);
  auxloader(p, oldFloppy);
}
/*                                                              ^ ^
The above functions pass device type and number to inthandler() | |
*/


void noWaitForIO(int dNum, int offset)
{
  //save completion status
  struct devSemStruct* dv = &devStruct[dNum + offset];
  dv->sem++;
  dv->status = dv->devReg->d_stat;
  dv->dadd = dv->devReg->d_amnt;
}

//unlock device
void handleUnlock(int dNum, int offset)
{
  struct devSemStruct* dv = &devStruct[dNum + offset];
  proc_t* p = headBlocked(&(dv->sem));
  p->p_s.s_r[2] = dv->devReg->d_dadd;
  p->p_s.s_r[3] = dv->devReg->d_stat;
  
  proc_t* q;
  intsemop(&dv->sem, UNLOCK);
}

/*
Saves completion status if waitforio call not recieved, or it does intsemop(Unlock)
on semaphore corresponding to device
*/
void static inthandler(int devType, int devNum)
{
  
  int offset;
  struct devSemStruct* dv;
  int currSem;
  
  //set variables
  if(devType == TERMINAL)
  {
    offset = 0; // 0 devices before terminal in devStruct
    dv = &devStruct[devNum + offset];
    currSem = dv->sem;
  }
  else if(devType == PRINTER)
  {
    offset = 5; // 5 devices before printer in devstruct
    dv = &devStruct[devNum + offset];
    currSem = dv->sem;
  }
  else if(devType == DISK)
  {
    offset = 7; //7 devices before disk in devStruct
    dv = &devStruct[devNum + offset];
    currSem = dv->sem;
  }
  else if(devType == FLOPPY)
  {
    offset = 11; //11 devices before floppy in devStruct
    dv = &devStruct[devNum + offset];
    currSem = dv->sem;
  }
  
  //handle
  if(currSem >= 0)
  {
    //if no wait for io call
    noWaitForIO(devNum, offset);
  }
  else
  {
    //call intsemop(UNLOCK)
    handleUnlock(devNum, offset);
  }
}


/*
if RQ not empty, put process at head to tail. Do semop on pseudoClock if necessary.
Then call schedule for p at head of RQ.
*/
void static intclockhandler()
{
  //interval timer has reached 0, thus clock interrupt
  intervalsLeft -=1; // another (100 ms / timeslice ) ms interval bites the dust
  int scheduleBool = FALSE;
  
	if ((headQueue(RQ)) != (proc_t*) ENULL)
  {
    int nonEmpty = TRUE;
    //if RQ nonempty, insert p at head to tail
    proc_t* p = removeProc(&RQ);
    stopTime(p);
    
    
    proc_t* proc;
    p->p_s = *oldClock;
    insertProc(&RQ, p);
  }
  
  //if necessary, do intsemop(UNLOCK) on pseudoclock (v'd every 100 ms)
  if(intervalsLeft <= 0)
  {
    proc_t* proc;
  
    intervalsLeft = (SECOND / 10)/timeslice; //100 ms broken up into 100 / timeslice intervals
    proc_t* blocked = headBlocked(&pseudoClock);
    if(blocked != (proc_t*) ENULL)
    {
      proc_t* removed = headBlocked(&pseudoClock);
      intsemop(&pseudoClock,UNLOCK);
    }  
  }
  //schedule process at RQ head
  schedule();
}

/*
Similar to semop call in traps. Has 2 args, addres of semaphore, and the op. Uses the
ASL and calls insertBlocked and removeBlocked
*/
void static intsemop(int* sem, int op)
{
  int scheduleBool = FALSE;
  //get sem and op values
  int oldSemVal = *(sem);
  int* currSemAddr = sem;
  //update semaphore value
  int newSemVal = oldSemVal + op;
  *(currSemAddr) = newSemVal;
  if(op == UNLOCK)
  {
    //if the semaphore is being freed up
    if (oldSemVal < 0)
    {
      proc_t* p;
      
      //if the semaphore had > 0 processes blocked on it
      proc_t* proc = removeBlocked(((int*)currSemAddr));
        
      if(proc->qcount == 0)
      {
        //if freed process not blocked on any queues
        insertProc(&RQ, proc);
      }
    }
  }
  else if(op == LOCK)
  {
    //if the semaphore is being blocked
    if(oldSemVal < 0 || oldSemVal == 0)
    {
      proc_t* proc = headQueue(RQ);
      scheduleBool = TRUE;
    }
  }

  if(scheduleBool == TRUE)
  {
    //need to schedule
    proc_t* p = removeProc(&RQ);
      
    if(p != (proc_t*) ENULL)
    {
      insertBlocked(((int*)currSemAddr), p);  
    }
    
    scheduleBool = FALSE;
    schedule();
  }
}


//ask TA HOW TO SLEEEEEEEEEEEEEEEPPPP!!!/////////////
/*
called when RQ empty. CPU halts until interrupt occurs.
*/
void static sleep()
{
  asm("  stop  #0x2000");
}
