//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel
  
/*
/*********************
This module handles system calls. There are seven functions corresponding to seven
system calls: creatproc, killproc, semop, notused, trapstate, getcputime, and 
trap-sysdefault. 
**********************/

#include "../../h/util.h"
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/main.e"


#include "../../h/int.e"

void kill(proc_t* toDie);
proc_t* getChild(proc_t* currPar);

/*
Causes new child process to spawn. D4 contains address of processor state area at 
time of execution. This state should be used as initial state for newly created 
process. The process executing SYS1 (this method) continues to exist and execute. If
new process cannot be created due to lack of resources (no more entries in proc table)
error code of -1 (ENULL) is returned in D2. Otherwise, d2 contains 0 upon return.
*/
void creatproc(state_t *oldTrapArea)
{
  
  //allocate new process and set its variables
  proc_t* child = allocProc();
  
  if(child == (proc_t*) ENULL)
  {
    //if not enough resources
    oldTrapArea->s_r[2] = ENULL;
  }
  else if(child != (proc_t *) ENULL)
  {
    //enough resources spawn process
    
    //D2 = 0
    oldTrapArea->s_r[2] = 0;
    
    //spawning process is process running
    proc_t* spawningProcess = headQueue(RQ);
    
    //set process state variables
    child->p_s = *((state_t *) oldTrapArea->s_r[4]);
    
    //put process in correct spot in tree
    if(spawningProcess->child != (proc_t*) ENULL)
    {
      //if already has child, traverse list
      proc_t* current = spawningProcess->child;
      while(current->sibling != (proc_t*) ENULL)
      {
        current = current->sibling;
      }
      //set pointers
      current->sibling = child;
      child->parent = spawningProcess;
      child->sibling = (proc_t*) ENULL;    
    }
    else
    {
      //first kid
      //set pointers
      spawningProcess->child = child;
      child->parent = spawningProcess;
      child->sibling = (proc_t*) ENULL;
    
    }
    
    //insert process into RQ
    insertProc(&RQ, child);
    
  }
}

/*
This causes executing process to die. Additionally, all child processes, and their
processes etc. etc. are killed. Execution doesn't complete untill all progeny are
killed.
*/
void killproc(state_t* oldTrapArea)
{
  //wrapper method
  proc_t* toBeKilled = headQueue(RQ);
	kill(toBeKilled);
  schedule();
}

void kill(proc_t* toDie)
{
  //check if has children
  proc_t *chProc = getChild(toDie);
  while(chProc != (proc_t *) ENULL)
  {
    //kill the kids
    kill(chProc);
    chProc = getChild(toDie);
  }

  //clear semvecs
  
  int blockingSem = FALSE;
	int i;
	for(i=0; i<SEMMAX; i++)
  {
    int* currSemVec = toDie->semvec[i];
		if(currSemVec != (int *) ENULL)
    {
      //free semaphores
      *currSemVec += 1;
      blockingSem = TRUE;	
		}
	}
 if(blockingSem == TRUE)
 {
   outBlocked(toDie);
 }
   
 //kill the process once it has no more kids
 if(toDie->parent != (proc_t*) ENULL)
 {
   
   //not the parent process
   //traverse to find removed element
   int hasSibling = FALSE;
   proc_t* traverser = toDie->parent->child;
   while(traverser->sibling != (proc_t*) ENULL)
   {
     if(traverser->sibling == toDie)
     {
       hasSibling = TRUE;
       break;
     }
     traverser = traverser->sibling;
   }
   
   if(hasSibling == TRUE)
   {
     //if has siblings, unlink it appropriately.
     traverser->sibling = toDie->sibling;
   }
   
   //remove
   toDie->parent = (proc_t*) ENULL;
   toDie->sibling = (proc_t*) ENULL;
   
     
   //cull
   outProc(&RQ,toDie);
   freeProc(toDie);
 }
 else
 {
   //parent process
   //cull 
   outProc(&RQ,toDie);
   freeProc(toDie);
 }
}

//return child to kill it and its kids
proc_t* getChild(proc_t* currPar)
{

  proc_t* killed = currPar->child;
  
  if(killed == (proc_t*) ENULL)
  {
      //all kids killed :o
      return killed;
  }
  else
  {
    //more to kill
    
    //unlink
    currPar->child = killed->sibling;
    killed->parent = (proc_t*) ENULL;
    killed->sibling =(proc_t*) ENULL;
    return killed;
  }
}

/*
When executed, applies a set of V and P operations to a group of semaphores, stored
with their ops in a vpop struct found in vpop.h. D4 contains vpop vector address, 
D3 contains number of vpops in vector.
*/
void semop(state_t* oldTrapArea)
{
  int scheduleBool = FALSE;
  
  //get values from registers
  vpop* current = (vpop*)oldTrapArea->s_r[4];
  int vpopNum = oldTrapArea->s_r[3];
  
  //traverse vector
  int i = 0;
  while(i < vpopNum)
  {
    //get sem and op values
    int op = current->op;
    int oldSemVal = *(current->sem);
    int* currSemAddr = (int*)current->sem;
    
    //update semaphore value
    int newSemVal = oldSemVal + op;
    *(currSemAddr) = newSemVal;
    
    if(op == UNLOCK)
    {
      //if the semaphore is being freed up
      if (oldSemVal < 0)
      {
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
        //if the prevSem has no free slots and must start a queue
        proc_t* proc = headQueue(RQ);
        proc->p_s = *oldTrapArea;
        insertBlocked(((int*)currSemAddr), proc);
        scheduleBool = TRUE;
      }
    }
    //go to next in array
    current = current + 1;
    i += 1;
  }

  if(scheduleBool == TRUE)
  {
    proc_t* p = removeProc(&RQ);
    schedule();
  }
 
}

/*
If instruction executed, should result in error condition.
Nucleus halts.
*/
void notused(state_t* oldTrapArea)
{
  HALT();
}

/*
Specifies type of trap (in D2), area where old state is stored when trap occur 
(in D3), and area that is to be the new processor state (in D4)
*/
void trapstate(state_t* oldTrapArea)
{
  proc_t* p = headQueue(RQ);
  int trapType = oldTrapArea->s_r[2];
  
  if(trapType == PROGTRAP)
  {
    //program trap
    if(p->progOld == (state_t*) ENULL)
    {
      p->progOld = (state_t*)oldTrapArea->s_r[3];
      p->progNew = (state_t*)oldTrapArea->s_r[4];
      
    }
    else
    {
      //if already executed it's a SYS2
      killproc(oldTrapArea);
      
    }
    
  }
  else if(trapType == MMTRAP)
  {
    //memory management trap
    if(p->mmOld == (state_t*) ENULL)
    {
      p->mmOld = (state_t*)oldTrapArea->s_r[3];
      p->mmNew = (state_t*)oldTrapArea->s_r[4];

    }
    else
    {
      //if already executed it's a SYS2
      killproc(oldTrapArea);
      
    } 
  }
  else if(trapType == SYSTRAP)
  {
    //program trap
    if(p->sysOld == (state_t*) ENULL)
    {
      p->sysOld = (state_t*)oldTrapArea->s_r[3];
      p->sysNew = (state_t*)oldTrapArea->s_r[4];
    }
    else
    {
      //if already executed it's a SYS2
      killproc(oldTrapArea);

    }
  }

}

/*
When executed, causes CPU time in microseconds of process executing it to be placed
into D2.
*/
void getcputime(state_t* oldTrapArea)
{
  proc_t* process = headQueue(RQ);
  oldTrapArea->s_r[2] = process->cpuTime;
}

