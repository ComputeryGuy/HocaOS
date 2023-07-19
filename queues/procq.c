//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel
  
/*
This program creates the abstraction of queues of processes.
The queues are circular queues.

Elements of process queues come from array procTable[MAXPROC]

Unused queue entries are kept on freelist beginning with procFree_h.
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.h"

proc_t procTable[MAXPROC];
proc_t* procFree_h = procTable;

//for panic fucntion
char msgbuf[128];


/*Insert element pointed to by p into process queue where
tp contains the pointer/index to the tail. Update tail pointer
accordingly. If process already in SEMMAX queues, call panic function */
insertProc(proc_link *tp, proc_t *p)
{

  int ti = tp->index;
  proc_t* tail = tp->next;
  
  //if proc allready in SEMMAX QUEUES, call PANIC FUNC
  if(p->qcount == SEMMAX)
  {
    panic("Process is already in the maximum amount of queues");
  }
  
  int i;
  for(i = 0; i < SEMMAX; i++)
  {
    if(p->p_link[i].next == (proc_t*) ENULL)
    {
      //if the spot is open
      
      if(tail == (proc_t*) ENULL)
      { 
        //if first added to queue
      
        //update
        p->p_link[i].index = i;
        p->p_link[i].next = p;
        tp->index = i;
        tp->next = p;
        p->qcount += 1;
    
        return;
      }
      else
      {
        //if not first in queue
        
        //add to end of queue
        proc_t* head = tail->p_link[ti].next;
        int hi = tail->p_link[ti].index;
        
        
        p->p_link[i].index = hi;
        p->p_link[i].next = head;
        
        tail->p_link[ti].index = i;
        tail->p_link[ti].next = p;
        
    
        p->qcount += 1;
    
        
        tp->index = i;
        tp->next = p;  
        
        return;   
      }
    
    }
  }
      
}

/* remove the first element from process queue whose tail is pointed to by tp.
Return ENULL if queue initially empty, otherwise return pointer to removed element.
Update pointer to tail of queue if necessary
*/
proc_t* removeProc(proc_link* tp)
{
  proc_t* tail = tp->next;
  int ti = tp->index;
  
  
  if(tail == (proc_t*)ENULL)
  {
    //if queue initially empty
    return (proc_t*) ENULL;  
  }
  
  proc_t* head = tail->p_link[ti].next;
  
  if(tail == head)
  {
    //if only one in queue
    tail->p_link[ti].index = ENULL;
    tail->p_link[ti].next = (proc_t*) ENULL;
    
    tp->index = ENULL;
    tp->next = (proc_t*) ENULL;
  }
  else
  {
    //if multiple in queue
    
    //unlink
    int hi = tail->p_link[ti].index;
    tail->p_link[ti].index = head->p_link[hi].index;
    tail->p_link[ti].next = head->p_link[hi].next;
    
    //update removed
    head->p_link[hi].index = ENULL;
    head->p_link[hi].next = (proc_t*) ENULL;    
  }

  head->qcount -= 1;
  
  return head;

}


/*Remove process table entry pointed to by p from q whose tail is
pointed to by tp. Update pointer to tail of queue if necessary. If desired
entry not in defined queue(an error cond) return ENULL. Otherwise, return p.
Note that p can point to any element of the queue.*/
proc_t* outProc(proc_link *tp, proc_t *p)
{
  

  proc_t* tail = tp->next;
  int ti = tp->index;
  
  if(tail == (proc_t*)ENULL)
  {
    //if queue initially empty
    return (proc_t*) ENULL;  
  }
  
  proc_t* head = tail->p_link[ti].next;
  
  if(tail == head)
  {
    //if only one in queue
    if(tail == p)
    {
      //if p found
      tail->p_link[ti].index = ENULL;
      tail->p_link[ti].next = (proc_t*) ENULL;
    
      tp->index = ENULL;
      tp->next = (proc_t*) ENULL;
      tail->qcount -= 1;
      
      return tail;
    }
    else
    {
      //if p not found
      return (proc_t*) ENULL;
    }
    
  }
  else
  {
    //if multiple in queue
    
    //search
    int ci = ti;
    int pi = ti;
    proc_t* current = tail;
    proc_t* prev = current;
    
    //current at head
    ci = current->p_link[pi].index;
    current = current->p_link[pi].next;
    
    while(current != tail)
    {
      //traverse the queue
      if(current == p)
      {
        //found, return

        //unlink
        prev->p_link[pi].index = current->p_link[ci].index;
        prev->p_link[pi].next = current->p_link[ci].next;
        
        current->p_link[ci].index = ENULL;
        current->p_link[ci].next = (proc_t*) ENULL;
        
        current->qcount -= 1;
        
        return current;
      }
      
      pi = ci;
      prev = current;
      ci = current->p_link[ci].index;
      current = current->p_link[pi].next;
    }
    
    if(current == p) 
    {
      //if tail
      //found, update queue and tp
      
      prev->p_link[pi].index = current->p_link[ci].index;
      prev->p_link[pi].next = current->p_link[ci].next;    
    
      //unlink
      current->p_link[ci].index = ENULL;
      current->p_link[ci].next = (proc_t*) ENULL;
      
      current->qcount -=1;
      
      //update tp
      tp->index = pi;
      tp->next = prev;
      
      return current;
      
    }
    else
    {
      //if p not found
      return (proc_t*) ENULL;
    }
    
     
  }
  
}

/* Return ENULL if the procFree list is empty.
Otherwise remove an element from procFree list and 
return a pointer to it.*/
proc_t* allocProc()
{
  proc_t* available;
  
  /*remove available element and update list */
  available = procFree_h;
  if(available == (proc_t*) ENULL)
  {
    /*if the list is empty return ENULL*/
    return available;
  }
  else
  {
    /*remove an element from the list and return*/
    procFree_h = procFree_h->p_link[0].next;
    
    available->p_link[0].index = ENULL;
    available->p_link[0].next = (proc_t*) ENULL;
    
    
    //added during nucleus phase to init process tree items to ENULL
    available->parent = (proc_t*) ENULL;
    available->child = (proc_t*) ENULL;
    available->sibling = (proc_t*) ENULL;
      
    //nucleus phase, state pointers init
    available->sysOld = (state_t*) ENULL;
    available->sysNew = (state_t*) ENULL;
    available->progOld = (state_t*) ENULL;
    available->progNew = (state_t*) ENULL;
    available->mmOld = (state_t*) ENULL;
    available->mmNew = (state_t*) ENULL;
    
    return available;
  }
}

/* Return the element pointed to by p into the procFree list. */
freeProc(proc_t *p)
{
  if(procFree_h == (proc_t*) ENULL)
  {
    /*if free list empty*/
    procFree_h = p;
  }
  else
  {
    proc_t* current;
    
    /*if the list is non-empty, traverse and add to end*/
    current = procFree_h;
    while(current->p_link[0].next != (proc_t*) ENULL)
    {
      current = current->p_link[0].next;
    }
    
    //update the value
    current->p_link[0].next = p;  
  }
  //unlink
    int i;
    for(i = 0; i < SEMMAX; i++)
    {
      p->p_link[i].index = ENULL;
      p->p_link[i].next = (proc_t*) ENULL;
      
      //added for asl.c
      p->semvec[i] = (int*) ENULL;
      
    }
    
    //added during nucleus phase to set process tree items to ENULL
    // during killproc
    p->parent = (proc_t*) ENULL;
    p->child = (proc_t*) ENULL;
    p->sibling = (proc_t*) ENULL;
    
    
    
    p->qcount = 0;
}

/*Return pointer to the process table entry at the head of the queue. The tail of the queue is pointed to by tp.*/
proc_t* headQueue(proc_link tp)
{
  
  if(tp.next == (proc_t*) ENULL)
  {
    //if queue is empty 
    
    return (proc_t*) ENULL;
  }
  
  //if queue non-empty
  proc_link current = tp;
  int i = current.index;
  current = (current.next)->p_link[i];
  return (proc_t*) (current.next);
  
}

/*Initialize the procFree list to contain
all the elements of the array procTable.
(Only called once during data structure initialization.)*/
initProc()
{

  int i;
  for(i = 0; i < MAXPROC - 1; i++)
  {
    procTable[i].p_link[0].next = &procTable[i+1];
    
    //initialize semvec vals and other p_links
    
    procTable[i].semvec[0] = (int*) ENULL;
    
    int j;
    for(j = 1; j < SEMMAX; j++)
    {
      procTable[i].semvec[j] = (int*) ENULL;
      procTable[i].p_link[j].next = (proc_t*) ENULL;
    }
  
    //added during nucleus phase to init process tree items to ENULL
    procTable[i].parent = (proc_t*) ENULL;
    procTable[i].sibling = (proc_t*) ENULL;
    procTable[i].child = (proc_t*) ENULL;
    
    
    //nucleus phase, sys pointers init
    procTable[i].sysOld = (state_t*) ENULL;
    procTable[i].sysNew = (state_t*) ENULL;
    procTable[i].progOld = (state_t*) ENULL;
    procTable[i].progNew = (state_t*) ENULL;
    procTable[i].mmOld = (state_t*) ENULL;
    procTable[i].mmNew = (state_t*) ENULL;

    //nucleus phase, for clock
    procTable[i].cpuTime = 0;
    procTable[i].startTime = 0;
    

  }
  
  //init last procTable entry
  for(i = 0; i < SEMMAX; i++)
  {
    procTable[MAXPROC - 1].p_link[i].next = (proc_t *) ENULL;
    procTable[MAXPROC - 1].semvec[i] = (int*) ENULL;
    
  }
  
   //added during nucleus phase to init process tree items to ENULL
    procTable[MAXPROC - 1].parent = (proc_t*) ENULL;
    procTable[MAXPROC - 1].child = (proc_t*) ENULL;
    procTable[MAXPROC - 1].sibling = (proc_t*) ENULL;
 
    //nucleus phase, sys pointers init
    procTable[MAXPROC - 1].sysOld = (state_t*) ENULL;
    procTable[MAXPROC - 1].sysNew = (state_t*) ENULL;
    procTable[MAXPROC - 1].progOld = (state_t*) ENULL;
    procTable[MAXPROC - 1].progNew = (state_t*) ENULL;
    procTable[MAXPROC - 1].mmOld = (state_t*) ENULL;
    procTable[MAXPROC - 1].mmNew = (state_t*) ENULL;

    //nucleus phase, for clock
    procTable[MAXPROC - 1].cpuTime = 0;
    procTable[MAXPROC - 1].startTime = 0;
}


/*
The panic function
*/
panic(s)
register char *s;
{
	register char *i=msgbuf;

	while ((*i++ = *s++) != '\0')
		
/*
	HALT();
*/
   asm("	trap	#0");
}



