//This code is my own work, it was written without consulting code written by other students
  //- Paul L. O'Friel
  
#include "../../h/slsyscall1.e"
#include "../../h/slsyscall2.e"
#include "../../h/page.e"

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/util.h"

#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/trap.e"
#include "../../h/main.e"

#include "../../h/int.e"

#include "h/tconst.h"

void static cron();
void static tprocess();

/*
To
start the execution of the bootcode you should set the PC to 0x80000 + 31 * PAGESIZE.
*/
int bootcode[] = {	/* boot strap loader object code */
0x41f90008,
0x00002608,
0x4e454a82,
0x6c000008,
0x4ef90008,
0x0000d1c2,
0x787fb882,
0x6d000008,
0x10bc000a,
0x52486000,
0xffde4e71
};

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");


/*
This function initializes all segment and page tables. In particular it
protects the nucleus from the support level by marking the nucleus
pages as not present. It initializes the page module by calling
pageinit(). It calls getfreeframe() and loads the bootcode on the page
frame. It passes control to p1a() which runs with memory mapping
on and interrupts enabled.
*/

void p1()
{
  pageinit();
  
  
  
}


/*
This function creates the terminal processes. Then it becomes the
cron process.
*/
void static p1a()
{

  state_t term;
  STST(&term);
  
  term.s_pc = (int)tprocess;
  term.s_sp = Tsysstack[0];
  r4 = (int)&term;
  SYS1();
  
  
  term.s_pc = (int)tprocess;
  term.s_sp = Tsysstack[1];
  r4 = (int)&term;
  SYS1();
  

  
  //becomes cron
  
  cron();
}

/*
This function does the appropriate SYS5s and loads a state with user
mode and PC=0x80000+31*PAGESIZE.
*/
void static tprocess()
{
  state_t proc;
  STST(&proc);
  
  
}

/*
This function checks the validity of a page fault. It calls getfreeframe() to allocate a free page frame. If necessary it calls
pagein() and then and it updates the page tables. It uses the semaphore sem_mm to control access to the critical section that updates
the shared data structures. The pages in segment two are a special
case in this function
*/
void static slmmhandler()
{


}

/*
This function has a switch statement and it calls the functions in
slsyscall1.c and slsyscall2.c
*/

void static slsyshandler()
{

}


/*
This function calls terminate()
*/
void static slproghandler()
{

}

/*
This function releases processes which delayed themselves, and it
shuts down if there are no T-processes running. cron should be in an
infinite loop and should block on the pseudoclock if there is no work
to be done. If possible you should synchronize delay and cron, otherwise one point will be lost.
*/
void static cron()
{

}

