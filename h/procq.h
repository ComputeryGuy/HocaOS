//added to allow compilation of program during nucleus phase
typedef struct proc_t proc_t;

/* link descriptor type */
typedef struct proc_link {
	int index;
	struct proc_t *next;
} proc_link;

/*process table entry type */
typedef struct proc_t {
	proc_link p_link[SEMMAX]; /*pointers to next entries */
	state_t   p_s;	/*processor state of the process */
	int qcount;	/* number of queues containing this entry */
	int *semvec[SEMMAX];	/*vector of active semaphores for this entry*/

  //Added for nucleus process creation/deletion
  proc_t* parent;
  proc_t* child;
  proc_t* sibling;
  
  //added for nucleus traps
  state_t* sysOld;
  state_t* sysNew;
  
  state_t* progOld;
  state_t* progNew;
  
  state_t* mmOld;
  state_t* mmNew;
  
  //added for nucleus cpu time
  int cpuTime;
  int startTime;
  int stopTime;
	/*plus other entries to be added by you later */
}proc_t;