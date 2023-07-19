//This code is my own work, it was written without consulting code written by
  //other students
  //- Paul L. O'Friel
/****************
This module handles the traps

****************/

#include "../../h/util.h"

#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"

#include "../../h/procq.e"
#include "../../h/asl.e"

#include "../../h/syscall.e"
#include "../../h/main.e"

#include "../../h/int.e"


/*
register (int)d2 asm("%d2");
register (int)d3 asm("%d3");
register (int)d4 asm("%d4");
*/


state_t* oldProg;
state_t* oldMM;
state_t* oldSys;

void static trapsyshandler();
void static trapmmhandler();
void static trapproghandler();

void auxTrapHandler(proc_t* p, state_t *oldstate, int encoding);
void startTime(proc_t * currProc);
void stopTime(proc_t * currProc);

/*
This function loads several entries in the EVT and it sets the new
areas for the traps.
*/
void trapinit()
{

  // load EVT entries
  *(int*)0x008 = (int)STLDMM;			/*   0x008            mm trap   */
  *(int*)0x00c = (int)STLDADDRESS;		/*   0x00c            prog trap */
  *(int*)0x010 = (int)STLDILLEGAL;		/*   0x010            prog trap */
  *(int*)0x014 = (int)STLDZERO;			/*   0x014            prog trap */
  *(int*)0x020 = (int)STLDPRIVILEGE;		/*   0x020            prog trap */
  *(int*)0x08c = (int)STLDSYS;			/*   0x08c            sys trap  */
  *(int*)0x94 = (int)STLDSYS9;			/*   0x94             sys trap  */
  *(int*)0x98 = (int)STLDSYS10;			/*   0x98             sys trap  */
  *(int*)0x9c = (int)STLDSYS11;			/*   0x9c             sys trap  */
  *(int*)0xa0 = (int)STLDSYS12;			/*   0xa0             sys trap  */
  *(int*)0xa4 = (int)STLDSYS13;			/*   0xa4             sys trap  */
  *(int*)0xa8 = (int)STLDSYS14;			/*   0xa8             sys trap  */
  *(int*)0xac = (int)STLDSYS15;			/*   0xac             sys trap  */
  *(int*)0xb0 = (int)STLDSYS16;			/*   0xb0             sys trap  */
  *(int*)0xb4 = (int)STLDSYS17;			/*   0xb4             sys trap  */
  *(int*)0x100 = (int)STLDTERM0;			/*   0x100            interrupt */
  *(int*)0x104 = (int)STLDTERM1;			/*   0x104            interrupt */
  *(int*)0x108 = (int)STLDTERM2;			/*   0x108            interrupt */
  *(int*)0x10c = (int)STLDTERM3;			/*   0x10c            interrupt */
  *(int*)0x110 = (int)STLDTERM4;			/*   0x110            interrupt */
  *(int*)0x114 = (int)STLDPRINT0;		/*   0x114            interrupt */
  *(int*)0x11c = (int)STLDDISK0;			/*   0x11c            interrupt */
  *(int*)0x12c = (int)STLDFLOPPY0;		/*   0x12c            interrupt */
  *(int*)0x140 = (int)STLDCLOCK;			/*   0x140            interrupt */

  //init old areas
  oldProg = (state_t*)0x800;
  oldMM = (state_t*)0x898;
  oldSys = (state_t*)0x930;
  
  //set new area for traps
  state_t* newProg = (oldProg + 1);
  state_t* newMM = (oldMM + 1);
  state_t* newSys = (oldSys + 1);
  
  //store
  //load new areas with PC of high level handlers
  STST(newProg);    
  newProg->s_pc = (int)trapproghandler;
  STST(newMM);
  newMM->s_pc = (int)trapmmhandler;
  STST(newSys);
  newSys->s_pc = (int)trapsyshandler;
}

/*
This function handles 9 different traps. It has a switch statement and each case calls
a function. Two of the functions, waitforp-clock and waitforio, are in int.c. The other
seven are in syscall.c. Note that trapsys handler passes a pointer to the old trap area
to the functions in syscall.c and int.c
*/
void static trapsyshandler()
{

  state_t* oldTrapArea = oldSys;
  proc_t* p = headQueue(RQ);
  int sysCall = oldTrapArea->s_tmp.tmp_sys.sys_no;
	stopTime(p);
  int privBit = oldTrapArea->s_sr.ps_s;
  
	if (sysCall <= PRIVILEGE)
  {
    if(privBit != 1)
    {
    	oldTrapArea->s_tmp.tmp_pr.pr_typ = PRIVILEGE;
	  	auxTrapHandler(p,oldSys,PROGTRAP);
    }
	}
	switch (sysCall) {
		case 1:
			creatproc(oldTrapArea);
			break;
		case 2:
			killproc(oldTrapArea);
			break;
		case 3:
			semop(oldTrapArea);
			break;
		case 4:
			notused(oldTrapArea);
			break;
		case 5:
			trapstate(oldTrapArea);
			break;
		case 6:
			getcputime(oldTrapArea);
			break;
		case 7:
			waitforpclock(oldTrapArea);
			break;
		case 8:
			waitforio(oldTrapArea);
			break;
		default:
			auxTrapHandler(p, oldTrapArea,SYSTRAP);
			break;

	}
	startTime(p);
	LDST(oldTrapArea);

}

/*
This function passes up the trap or terminates the process.
*/
void static trapmmhandler()
{

	proc_t* p = headQueue(RQ);
	auxTrapHandler(p, oldMM, MMTRAP);

}
/*
This function passes up the trap or terminates the process.
*/
void static trapproghandler()
{
	proc_t* p = headQueue(RQ);
	auxTrapHandler(p, oldProg, PROGTRAP);
}

/*
Auxillary function to 'pass up' as reccommended by the TA
*/
void auxTrapHandler(proc_t* p, state_t *oldstate, int encoding)
{

  //stop in case terminated
  stopTime(p);
  if(encoding == PROGTRAP)
  {
    
    if (p->progNew == (state_t *) ENULL)
    {
      //if SYS5 not executed, run SYS2
      proc_t* proc;
      killproc(oldstate);
    } 
    else
    {
      int x;
      *p->progOld = *oldstate;
		  startTime(p);
      LDST(p->progNew);
     }
  }
	else if (encoding == MMTRAP)
  {
    if (p->mmNew == (state_t *) ENULL)
    {
      //if SYS5 not executed, run SYS2
      proc_t* proc;
      killproc(oldstate);
    } 
    else
    {
      int x;
      *p->mmOld = *oldstate;
		  startTime(p);
      LDST(p->mmNew);
			
     }
  }
  else if(encoding == SYSTRAP)
  {
    if (p->sysNew == (state_t *) ENULL)
    {
      //if SYS5 not executed, run SYS2
      proc_t* proc;
      killproc(oldstate);
    } 
    else
    {
      int x;
      *p->sysOld = *oldstate;
		  startTime(p);
      LDST(p->sysNew);
			
     }
  
  }	
			
}

void startTime(proc_t * currProc)
{
	int start;
	STCK(&start);
	currProc->startTime = start;
}

void stopTime(proc_t *currProc)
{
	int stop;
	STCK(&stop);
	currProc->cpuTime += stop - currProc->startTime;  
}

