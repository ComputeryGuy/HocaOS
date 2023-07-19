//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel


/*
This program creates the active semaphor list (ASL) abstraction as a doubly linked list,
sorted by semaphore address.

Elements of the ASL come from semdTable[MAXPROC].

Unused elements are kept on free list semdFree_h.
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.e"
#include "../h/asl.h"


semd_t semdTable[MAXPROC];
semd_t* semd_h = (semd_t*) ENULL;
semd_t* semdFree_h;



/* Insert process table entry p at tail of process queue associated with 
semaphore whose address is semAdd. If the semaphore is currently not active,
(no descriptor for it in ASL), allocate new descriptor from free list, insert it into
ASL (at appropriate position), and initialize all of the fields.
If new semaphore descriptor needs to be allocated and teh free list is empty,
return TRUE.

In all other cases return FALSE.
*/
insertBlocked(int* semAdd, proc_t* p)
{
  
  semd_t* current = semdFree_h;
  
  if(semd_h == (semd_t*) ENULL)
  {
    /* semaphore list empty */
    
    //allocate and insert
    semdFree_h = semdFree_h->s_next;
    semdFree_h->s_prev = (semd_t*) ENULL;
    
    semd_h = current;
    current->s_next = (semd_t*)ENULL;
    current->s_prev = (semd_t*)ENULL;
    current->s_semAdd = semAdd;
    
    proc_link* tail = &(current->s_link);
    insertProc(tail, p);
    
    int i;
    for(i = 0; i < SEMMAX; i++)
    {
      if(p->semvec[i] == (int*) ENULL)
      {
        p->semvec[i] = semAdd;
        return FALSE;
      }
    }   
    
  }
  else
  {
    /* semaphore list nonempty*/
    current = semd_h;
    semd_t* tail;
    while(current != (semd_t*) ENULL)
    {
      if(current->s_semAdd == semAdd)
      {
        /* if semaphore in list */
        
        //insert 
        proc_link* tail = &(current->s_link);
        insertProc(tail, p);
        
        int i;
        for(i = 0; i < SEMMAX; i++)
        {
          if(p->semvec[i] == (int*) ENULL)
          {
            p->semvec[i] = semAdd;
            return FALSE;
          }
        }
      }
      tail = current;
      current = current->s_next;
    
    }
    
    /* if semaphore not in list (must allocate now) */
    
    if(semdFree_h == (semd_t*) ENULL)
    {
      /* if semaphore free list empty*/
      return TRUE;
    }
       
    //allocate and insert
    current = semdFree_h;
    
    
    semdFree_h = semdFree_h->s_next;
    
    if(semdFree_h != (semd_t*) ENULL)
    {
      semdFree_h->s_prev = (semd_t*) ENULL;
    }
    //
    current->s_next = (semd_t*) ENULL;
    current->s_prev = tail;
    tail->s_next = current;
    current->s_semAdd = semAdd;
    
    proc_link* qTail = &(current->s_link);
    insertProc(qTail, p);
    
    int i;
    for(i = 0; i < SEMMAX; i++)
    {
      if(p->semvec[i] == (int*) ENULL)
      {
        p->semvec[i] = semAdd;
        return FALSE;
      }
    }
     
  }

}

/* search ASL for semaphore. If none found, return ENULL. Otherwise, remove first 
process table entry from relevant process queue and return pointer. If the resultant
process queue for the semaphore becomes empty, remove descriptor from ASL and return to 
the free list.
*/
proc_t* removeBlocked(int* semAdd)
{
  proc_t* val;
  
  semd_t* current = semd_h;
  
  while(current != (semd_t*) ENULL)
  {
    if(current->s_semAdd == semAdd)
    {
      //if found
      val = removeProc(& current->s_link);
      int i;
      for(i = 0; i < SEMMAX; i++)
      {
        if(val->semvec[i] == semAdd)
        {
          //remove active semaphor vector entry
          val->semvec[i] = (int*) ENULL;
          break;
        }
        
      }
      
      if(headQueue(current->s_link) == (proc_t*) ENULL)
      {
        //if process queue now empty
        
        //return sem to freelist
                
        semd_t* prev = current->s_prev;
        semd_t* next = current->s_next;
        
        if(prev == (semd_t*) ENULL && next != (semd_t*) ENULL)
        {
          /* if val is head */
          next->s_prev = (semd_t*) ENULL;
          semd_h = next;          
        }
        else if(prev != (semd_t*) ENULL && next != (semd_t*) ENULL)
        {
          /* if val is middle*/
          prev->s_next = next;
          next->s_prev = prev;          
        }
        else if(prev != (semd_t*) ENULL && next == (semd_t*) ENULL)
        {
          /* if val is end */
          prev->s_next = (semd_t*) ENULL;
        }
        else
        {
          /*val is last value and asl now empty*/
          semd_h = (semd_t*) ENULL;
        }      
        
        current->s_prev = (semd_t*) ENULL;
        if(semdFree_h != (semd_t*) ENULL)
        {
          //if the free list is not empty
          current->s_next = semdFree_h;
          current->s_prev = (semd_t*) ENULL; 
          semdFree_h->s_prev = current;
        }
        else
        {
          //if the free list is empty
          current->s_next = (semd_t*) ENULL;
          current->s_prev = (semd_t*) ENULL;                
        }
        semdFree_h = current;
        
        current->s_semAdd = (int*) ENULL;
      }

      return val;
    }
  
    current = current->s_next;
  }

  //not found
  return (proc_t*) ENULL;

}

/* Remove process table entry p from queues associated with the correct
 semaphores on the ASL. If given p does not appear in any process queues 
(an error condition), return ENULL. Else, return p.
*/
proc_t* outBlocked(proc_t* p)
{
  int hasAppeared = FALSE;
  semd_t* current;
  proc_t* val;
  
  int i;
  for(i = 0; i < SEMMAX; i++)
  {
    /* check semvec for correct semaphores */
    if(p->semvec[i] != (int*) ENULL)
    {
      //if p is associated with this queue/semaphore
      hasAppeared = TRUE;
      
      int* address = p->semvec[i];
      current = semd_h;
      while(current != (semd_t*) ENULL)
      {
        if(current->s_semAdd == address)
        {
          //if relevant semaphore
          
          proc_link* tail = &(current->s_link);
          val = outProc(tail, p);
          
  
          if(headQueue(current->s_link) == (proc_t*) ENULL)
          {
            //if process queue now empty
        
            //return to freelist
                
            semd_t* prev = current->s_prev;
            semd_t* next = current->s_next;
        
            if(prev == (semd_t*) ENULL && next != (semd_t*) ENULL)
            {
              /* if val is head */
              next->s_prev = (semd_t*) ENULL;
              semd_h = next;          
            }
            else if(prev != (semd_t*) ENULL && next != (semd_t*) ENULL)
            {
              /* if val is middle*/
              prev->s_next = next;
              next->s_prev = prev;          
            }
            else if(prev != (semd_t*) ENULL && next == (semd_t*) ENULL)
            {
              /* if val is end */
              prev->s_next = (semd_t*) ENULL;
            }
            else
            {
              /*val is last value and asl now empty*/
              semd_h = (semd_t*) ENULL;
            }      
        
            current->s_prev = (semd_t*) ENULL;
            if(semdFree_h != (semd_t*) ENULL)
            {
              //if the list is not empty
              current->s_next = semdFree_h;
              current->s_prev = (semd_t*) ENULL; 
              semdFree_h->s_prev = current;
            }
            else
            {
              //if the list is empty
              current->s_next = (semd_t*) ENULL;
              current->s_prev = (semd_t*) ENULL;                
            }
            
            semdFree_h = current;
            current->s_semAdd = (int*) ENULL;
          }
          break;
        }
        
        current = current->s_next;
      }  
      
      //free semvec
      p->semvec[i] = (int*) ENULL;
     
    }

  }
  
  if(hasAppeared == FALSE)
  {
    return (proc_t*) ENULL;
  }
  else
  {
    return p;
  }

}

/* return a pointer to process table entry at head of process queue associated
with semaphore semAdd. If list empty, return ENULL*/
proc_t* headBlocked(int* semAdd)
{
  semd_t* current = semd_h;
  
  //if list nonempty (skips if list empty)
  while(current != (semd_t*) ENULL)
  {
    if(current->s_semAdd == semAdd)
    {
      //if found
      return headQueue(current->s_link);
    }
    current = current->s_next;
  }
  
  //if list empty 
  return (proc_t*) ENULL;
  
}

/* Initialize the semaphore descriptor free list */
initSemd()
{

  semdTable[0].s_prev = (semd_t*) ENULL;
  
  int i;
  for(i = 0; i < MAXPROC - 1; i++)
  {
    semdTable[i].s_next = &semdTable[i+1];
    semdTable[i+1].s_prev = &semdTable[i];
    semdTable[i].s_link.index = ENULL;
    semdTable[i].s_link.next = (proc_t*) ENULL;
    
  }
  
  semdTable[MAXPROC - 1].s_next = (semd_t*) ENULL;
  semdTable[MAXPROC - 1].s_link.index = ENULL;
  semdTable[MAXPROC - 1].s_link.next = (proc_t*) ENULL;
  
  semdFree_h = semdTable;
}

/* determines if any semaphores on ASL. Returns FALSE if ASL is empty,
true if ASL is not empty */
headASL()
{
  if(semd_h == (semd_t*) ENULL)
  {
    // if ASL empty
    return FALSE;
  }
  else
  {
    // if ASL not empty
    return TRUE;
  }


}