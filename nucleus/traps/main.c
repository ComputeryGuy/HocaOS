//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel
  
/****************
This module coordinates the initialization of the nucleus and starts execution of the 
first process, p1(). It also provides a scheduling function. The module contains two 
functions: main() and init(). init() is static. It also contains a function that
exports schedule().

*****************/
#include "../../h/util.h"

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"

#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/trap.e"
#include "../../h/main.e"
#include "../../h/int.e"


void static init();
void schedule();

extern int p1();

state_t state;
proc_link RQ = {ENULL, (proc_t*) ENULL};
int memAmnt;

//new.s_pc = (int)p1;

void main()
{
  init();
  
  proc_t* p = allocProc();
  STST(&p->p_s);
  
  //allocate two pages physical memory
  memAmnt = state.s_sp; 
  memAmnt = memAmnt - (PAGESIZE + PAGESIZE);
  
  p->p_s.s_sp = memAmnt;
  p->p_s.s_pc = (int)p1;
  
  insertProc(&RQ, p);
  schedule();
   
}

/*
This function determines how much physical memory there is in the system.
It then calls initProc(), initSemd(), trapinit(), and intinit();
*/
void static init()
{
  STST(&state);
  memAmnt = state.s_sp;
  initProc();
  initSemd();
  trapinit();
  intinit();
}

/*
If the RQ is not empty this function calls intschedule() and loads the state
of the process at head of RQ. if RQ is empty it calls intdeadlock()
*/
void schedule()
{
  //check if RQ empty
  proc_t* rq = headQueue(RQ);
  if( rq != (proc_t*) ENULL)
  {
    //if non empty
    intschedule();
    startTime(rq);
    LDST(&rq->p_s);
  }
  else
  {
    //if empty
    intdeadlock();
  }

}


