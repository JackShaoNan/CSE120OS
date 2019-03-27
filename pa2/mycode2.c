/* mycode2.c: your portion of the kernel
 *
 *   	Below are procedures that are called by other parts of the kernel. 
 * 	Your ability to modify the kernel is via these procedures.  You may
 *  	modify the bodies of these procedures any way you wish (however,
 *  	you cannot change the interfaces).  
 */

#include "aux.h"
#include "sys.h"
#include "mycode2.h"

#define TIMERINTERVAL 1	// in ticks (tick = 10 msec)
#define L 100000 // for stride scheduling

/* 	A sample process table. You may change this any way you wish. 
 */

static struct {
	int valid;		// is this entry valid: 1 = yes, 0 = no
	int pid;		// process ID (as provided by kernel)
	int cpu;		// booked cpu
	int request;	// 1 for has required cpu, 0 for not
	int stride;		
	int pass;
	//int callTime; 	// for test
} proctab[MAXPROCS];

// vars for proportional
int freeCpu; 		// free cpu
int reqCpu; 		// booked cpu
int numOfFreeProc; 	// number of procs who doesn't book cpu

/* a circle array for queue and stack and round-robin
 */
 static struct {
	 int valid;
	 int pid;
 } list[MAXPROCS];
 int head; 			// first elmt in list
 int tail; 			// last elmt in list
 int size; 			// size of list
 
 int curIndex; // index for track the process being excuted in rr

 // helper func for proportional
 void distributeCpu();
 void resetPass();

/* 	InitSched () is called when the kernel starts up.  First, set the
 *  	scheduling policy (see sys.h). Make sure you follow the rules
 *   	below on where and how to set it.  Next, initialize all your data
 * 	structures (such as the process table).  Finally, set the timer
 *  	to interrupt after a specified number of ticks. 
 */

void InitSched ()
{
	int i;

	/* First, set the scheduling policy. You should only set it
	 * from within this conditional statement.  While you are working
	 * on this assignment, GetSchedPolicy () will return NOSCHEDPOLICY. 
	 * Thus, the condition will be true and you may set the scheduling
	 * policy to whatever you choose (i.e., you may replace ARBITRARY).  
	 * After the assignment is over, during the testing phase, we will
	 * have GetSchedPolicy () return the policy we wish to test.  Thus
	 * the condition will be false and SetSchedPolicy (p) will not be
	 * called, thus leaving the policy to whatever we chose to test
	 * (and so it is important that you NOT put any critical code in
	 * the body of the conditional statement, as it will not execute when
	 * we test your program). 
	 */
	if (GetSchedPolicy () == NOSCHEDPOLICY) {	// leave as is
		SetSchedPolicy (PROPORTIONAL);		// set policy here
	}

	// init vars
	head = -1;
	tail = -1;
	size = 0;
	curIndex = -1;

	freeCpu = 100;
	reqCpu = 0;
	numOfFreeProc = 0;

		
	/* Initialize all your data structures here */
	for (i = 0; i < MAXPROCS; i++) {
		proctab[i].valid = 0;
		list[i].valid = 0;

		proctab[i].cpu = 0;
		proctab[i].pass = 0;
		proctab[i].request = 0;
		proctab[i].stride = 0;

		//proctab[i].callTime = 0;
	}

	/* Set the timer last */
	SetTimer (TIMERINTERVAL);
	//DPrintf("init sched\n");
}


/*  	StartingProc (p) is called by the kernel when the process
 * 	identified by PID p is starting. This allows you to record the
 * 	arrival of a new process in the process table, and allocate
 *  	any resources (if necessary).  Returns 1 if successful, 0 otherwise. 
 */

int StartingProc (p)
	int p;				// process that is starting
{
	//DPrintf("\nstarting proc <%d>\n", p);
	int i;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (! proctab[i].valid) {
				proctab[i].valid = 1;
				proctab[i].pid = p;
				return (1);
			}
		}
		break;

	case FIFO:

		/* your code here */
		if (size < MAXPROCS) {
			if (size == 0) {
				head = 0;
				tail = 0;
			} else {
				tail = (tail + 1) % MAXPROCS;
			}
			list[tail].valid = 1;
			list[tail].pid = p;
			size++;
			return (1);
		}
		break;

	case LIFO:

		/* your code here */
		if (size < MAXPROCS) {
			if (size == 0) {
				head = 0;
				tail = 0;
			} else {
				tail = (tail + 1) % MAXPROCS;
			}
			list[tail].valid = 1;
			list[tail].pid = p;
			size++;
			DoSched();
			return (1);
		}
		break;

	case ROUNDROBIN:

		/* your code here */
		if (curIndex == -1) {
			curIndex = 0;
			proctab[curIndex].valid = 1;
			proctab[curIndex].pid = p;
			return (1);
		}
		// try to find a position for new proc from current index
		for (i = 1; i < MAXPROCS; ++i) {
			int index = (curIndex + i) % MAXPROCS;
			if (! proctab[index].valid) {
				proctab[index].valid = 1;
				proctab[index].pid = p;
				return (1);
			}
		}
		break;

	case PROPORTIONAL:

		/* your code here */
		for (i = 0; i < MAXPROCS; ++i) {
			if (!proctab[i].valid) {
				proctab[i].valid = 1;
				proctab[i].pid = p;
				numOfFreeProc++;
				if (numOfFreeProc > 0)
					distributeCpu();
				return (1);
			}
		}
		break;

	}


	DPrintf ("Error in StartingProc: no free table entries\n");
	return (0);
}
			

/*   	EndingProc (p) is called by the kernel when the process
 * 	identified by PID p is ending.  This allows you to update the
 *  	process table accordingly, and deallocate any resources (if
 *  	necessary).  Returns 1 if successful, 0 otherwise. 
 */


int EndingProc (p)
	int p;				// process that is ending
{
	//DPrintf("ending proc <%d>\n", p);
	int i;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid && proctab[i].pid == p) {
				proctab[i].valid = 0;
				return (1);
			}
		}
		break;

	case FIFO:

		/* your code here */
		list[head].valid = 0;
		head = (head + 1) % MAXPROCS;
		size--;
		return (1);
		break;

	case LIFO:

		/* your code here */
		list[tail].valid = 0;
		tail = (tail - 1 + MAXPROCS) % MAXPROCS;
		size--;
		return (1);
		break;

	case ROUNDROBIN:

		/* your code here */


		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid && proctab[i].pid == p) {
				proctab[i].valid = 0;
				return (1);
			}
		}

		break;

	case PROPORTIONAL:

		/* your code here */
		for (i = 0; i < MAXPROCS; ++i) {
			if (proctab[i].valid && proctab[i].pid == p) {
				if (proctab[i].request) {
					reqCpu -= proctab[i].cpu;
					freeCpu += proctab[i].cpu;
					proctab[i].request = 0;
				} else {
					numOfFreeProc -= 1;
				}
				if (numOfFreeProc > 0)
					distributeCpu();
				proctab[i].valid = 0;
				proctab[i].cpu = 0;
				proctab[i].pass = 0;
				proctab[i].stride = 0;
				return (1);
			}
		}

		break;

	}

	DPrintf ("Error in EndingProc: can't find process %d\n", p);
	return (0);
}


/* 	SchedProc () is called by kernel when it needs a decision for
 * 	which process to run next. It calls the kernel function
 *  	GetSchedPolicy () which will return the current scheduling policy
 *   	which was previously set via SetSchedPolicy (policy).  SchedProc ()
 * 	should return a process PID, or 0 if there are no processes to run. 
 */




int SchedProc ()
{
	
	int i;
	// For proportion: find the proc with minimum pass
	int minPass = 2 * L;
	int minPassIndex = -1;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid) {
				return (proctab[i].pid);
			}
		}
		break;

	case FIFO:

		/* your code here */
		if (size > 0) {
			return (list[head].pid);
		}
		break;

	case LIFO:

		/* your code here */
		if (size > 0) {
			return (list[tail].pid);
		}
		break;

	case ROUNDROBIN:

		/* your code here */
		for (i = 1; i <= MAXPROCS; ++i) {
			if (proctab[(curIndex + i) % MAXPROCS].valid) {
				curIndex = (curIndex + i) % MAXPROCS;
				//DPrintf("sched proc <%d>\n", proctab[curIndex].pid);
				return (proctab[curIndex].pid);
			}
		}
		break;

	case PROPORTIONAL:

		/* your code here */
		
		for (i = 0; i < MAXPROCS; ++i) {
			if (proctab[i].valid && proctab[i].pass < minPass) {
				minPass = proctab[i].pass;
				minPassIndex = i;
			}
		}
		// reset pass in case overflow
		if (minPass >= L) {
			// for (i = 0; i < MAXPROCS; ++i) {
			// 	if (proctab[i].valid) {
			// 		DPrintf("pid: %d, pass:%d\n", proctab[i].pid, proctab[i].callTime);
			// 	}
			// }
			resetPass();
		}
			
		proctab[minPassIndex].pass += proctab[minPassIndex].stride;
		//DPrintf("\npid: %d, pass:%d\n", proctab[minPassIndex].pid, proctab[minPassIndex].pass);
		//proctab[minPassIndex].callTime++;
		return (proctab[minPassIndex].pid);
		break;

	}
	
	return (0);
}


/*  	HandleTimerIntr () is called by the kernel whenever a timer
 *  	interrupt occurs.  Timer interrupts should occur on a fixed
 * 	periodic basis.
 */

void HandleTimerIntr ()
{
	//DPrintf("handle timer intr\n");
	SetTimer (TIMERINTERVAL);

	switch (GetSchedPolicy ()) {	// is policy preemptive?

	case ROUNDROBIN:		// ROUNDROBIN is preemptive
	case PROPORTIONAL:		// PROPORTIONAL is preemptive

		DoSched ();		// make scheduling decision
		break;

	default:			// if non-preemptive, do nothing
		break;
	}
}

/* 	MyRequestCPUrate (p, n) is called by the kernel whenever a process
 *  	identified by PID p calls RequestCPUrate (n).  This is a request for
 *   	n% of CPU time, i.e., requesting a CPU whose speed is effectively
 * 	n% of the actual CPU speed. Roughly n out of every 100 quantums
 *  	should be allocated to the calling process. n must be at least
 *  	0 and must be less than or equal to 100.  MyRequestCPUrate (p, n)
 * 	should return 0 if successful, i.e., if such a request can be
 * 	satisfied, otherwise it should return -1, i.e., error (including if
 *  	n < 0 or n > 100). If MyRequestCPUrate (p, n) fails, it should
 *   	have no effect on scheduling of this or any other process, i.e., AS
 * 	IF IT WERE NEVER CALLED.
 */

int MyRequestCPUrate (p, n)
	int p;				// process whose rate to change
	int n;				// percent of CPU time
{
	/* your code here */
	if (GetSchedPolicy() != PROPORTIONAL || n < 0 || n > 100)
		return (-1);
	
	// find p
	int index;
	for (index = 0; index < MAXPROCS; ++index) {
		if (proctab[index].pid == p) {
			break;
		}
	}

	if (n == 0) {
		if (proctab[index].request == 1) {
			proctab[index].request = 0;
			freeCpu += proctab[index].cpu;
			reqCpu -= proctab[index].cpu;
			proctab[index].cpu = 0;
			numOfFreeProc += 1;
		}
	} else if (proctab[index].request == 0 && n <= freeCpu) {
		// new request
		freeCpu -= n;
		reqCpu += n;
		numOfFreeProc -= 1;
		proctab[index].stride = L / n;
		proctab[index].request = 1;
		proctab[index].cpu = n;
	} else if (proctab[index].request == 1 && n <= freeCpu + proctab[index].cpu) {
		// update previous request
		freeCpu = freeCpu + proctab[index].cpu - n;
		reqCpu = reqCpu - proctab[index].cpu + n;
		proctab[index].stride = L / n;
		proctab[index].cpu = n;
	} else 
		return (-1);
	
	if (numOfFreeProc > 0)
		distributeCpu();

	return (0);
}


/*
help distribute free cpu to procs that doesn't request cpu
*/
void distributeCpu() {
	int cpuShare = 0;
	
	if (freeCpu != 0) {
		cpuShare = freeCpu / numOfFreeProc;
	}

	for (int i = 0; i < MAXPROCS; ++i) {
		if (proctab[i].valid && proctab[i].request == 0) {
			if (cpuShare == 0) {
				proctab[i].cpu = cpuShare;
				proctab[i].pass = L;
			} else {
				proctab[i].cpu = cpuShare;
				proctab[i].stride = L / cpuShare;
			}
		}
	}
}

/*
helper reset all pass value
*/
void resetPass() {
	for (int i = 0; i < MAXPROCS; ++i) {
		if (proctab[i].valid && proctab[i].cpu != 0)
			proctab[i].pass -= L;
	}
}
