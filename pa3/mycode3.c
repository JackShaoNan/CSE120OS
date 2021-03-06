/* mycode3.c: your portion of the kernel
 *
 *   	Below are procedures that are called by other parts of the kernel. 
 * 	Your ability to modify the kernel is via these procedures.  You may
 *  	modify the bodies of these procedures any way you desire (however,
 *  	you cannot change the interfaces).  
 */

#include "aux.h"
#include "sys.h"
#include "mycode3.h"

#define FALSE 0
#define TRUE 1

/* 	A sample semaphore table. You may change this any way you wish. 
 */

static struct {
	int valid;	// Is this a valid entry (was sem allocated)?
	int value;	// value of semaphore
	int waitprocs[MAXPROCS]; // a queue of waiting procs, FIFO
	int waitnum; // how many procs are waiting
	int head; // head of queue
	int tail; // tail of queue
} semtab[MAXSEMS];


/* 	InitSem () is called when kernel starts up.  Initialize data
 *  	structures (such as semaphore table) and call any initialization
 *   	procedures here. 
 */

void InitSem ()
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {		// mark all sems free
		semtab[s].valid = FALSE;
		semtab[s].waitnum = 0;
	}
}

/* 	MySeminit (p, v) is called by the kernel whenever the system
 *  	call Seminit (v) is called.  The kernel passes the initial
 *  	value v, along with the process ID p of the process that called
 * 	Seminit.  MySeminit should allocate a semaphore (find free entry
 * 	in semtab and allocate), initialize that semaphore's value to v,
 *  	and then return the ID (i.e., index of allocated entry). 
 */

int MySeminit (int p, int v)
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {
		if (semtab[s].valid == FALSE) {
			break;
		}
	}
	if (s == MAXSEMS) {
		DPrintf ("No free semaphores.\n");
		return (-1);
	}

	semtab[s].valid = TRUE;
	semtab[s].value = v;

	return (s);
}

/*   	MyWait (p, s) is called by the kernel whenever the system call
 * 	Wait (s) is called. 
 */

void MyWait (p, s)
	int p;				// process
	int s;				// semaphore
{
	/* modify or add code any way you wish */
	semtab[s].value--;
	if (semtab[s].value < 0) {
		// add p to waiting queue
		if (semtab[s].waitnum == 0) {
			semtab[s].head = 0;
			semtab[s].tail = 0;
			semtab[s].waitprocs[0] = p;
		} else {
			semtab[s].tail = (semtab[s].tail + 1) % MAXPROCS; 
			semtab[s].waitprocs[semtab[s].tail] = p;
		}
		semtab[s].waitnum++;
		Block(p);
	}
}

/*  	MySignal (p, s) is called by the kernel whenever the system call
 *  	Signal (s) is called.  
 */

void MySignal (p, s)
	int p;				// process
	int s;				// semaphore
{
	/* modify or add code any way you wish */
	semtab[s].value++;
	if (semtab[s].waitnum > 0) {
		semtab[s].waitnum--;
		int unblockme = semtab[s].waitprocs[semtab[s].head];
		if (semtab[s].waitnum == 1) {
			semtab[s].head = semtab[s].tail;
		} else if (semtab[s].waitnum > 1) {
			semtab[s].head = (semtab[s].head + 1) % MAXPROCS;
		}
		Unblock(unblockme);
	}
}

