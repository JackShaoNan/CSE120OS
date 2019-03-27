/*   	Umix thread package
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mycode4.h"
#include "string.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0
static int curThread = 0;
static int preThread;

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;			// current context
	jmp_buf env2;
	void (*f)();
	int p;
} thread[MAXTHREADS];

static struct queue
{
	/* data */
	int valid;
	int pre;
	int next;
}queue[MAXTHREADS];

static int head = -1;
static int tail = -1;
static int size = 0;

void addHead(int n) {
	queue[n].valid = 1;
	queue[n].pre = -1;
	queue[n].next = head;
	if (size != 0)
		queue[head].pre = n;
	else
		tail = n;
	head = n;
	size++;
}

void addTail(int n) {
	queue[n].valid = 1;
	queue[n].pre = tail;
	queue[n].next = -1;
	if (size != 0)
		queue[tail].next = n;
	else
		head = n;
	tail = n;
	size++;
}

void delete(int n) {
	queue[n].valid = 0;
	if (size == 1) {
		head = -1;
		tail = -1;
	} else if (head == n) {
		head = queue[n].next;
		queue[head].pre = -1;
	} else if (tail == n) {
		tail = queue[n].pre;
		queue[tail].next = -1;
	} else {
		queue[queue[n].pre].next = queue[n].next;
		queue[queue[n].next].pre = queue[n].pre;
	}
	size--;
}

#define STACKSIZE	65536		// maximum size of thread stack

/* 	MyInitThreads () initializes the thread package. Must be the first
 *  	function called by any user program that uses the thread package.  
 */

void MyInitThreads ()
{
	int i;
	int t1;
	int t2;

	if (MyInitThreadsCalled) {		// run only once
		Printf ("MyInitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	// initialize thread table
		thread[i].valid = 0;
		queue[i].valid = 0;
		queue[i].pre = -1;
		queue[i].next = -1;
	}

	thread[0].valid = 1;			// initialize thread 0
	addHead(0);

	if ((t1 = setjmp(thread[0].env2)) == 0) {
		for (i = 1; i <= MAXTHREADS; ++i) {
			char s[i * STACKSIZE];
			if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
				Printf ("Stack space reservation failed\n");
				Exit ();
			}
			if ((t2 = setjmp(thread[i].env2)) != 0) {
				(*thread[t2 - 1].f) (thread[t2 - 1].p);
				MyExitThread();
			}
		}
	} else {
		(*thread[0].f) (thread[0].p);
		MyExitThread();
	}

	MyInitThreadsCalled = 1;
}

/*  	MyCreateThread (f, p) creates a new thread to execute
 * 	f (p), where f is a function with no return value and
 * 	p is an integer parameter.  The new thread does not begin
 *  	executing until another thread yields to it. 
 */

int MyCreateThread (f, p)
	void (*f)();			// function to be executed
	int p;				// integer parameter
{
	int i;
	int t;

	if (! MyInitThreadsCalled) {
		Printf ("MyCreateThread: Must call MyInitThreads first\n");
		Exit ();
	}

	static int cur = 0;
	cur = (cur +1) % MAXTHREADS;
	int start = cur;
	while (thread[cur].valid == 1) {
		cur = (cur + 1) % MAXTHREADS;
		if (cur == start)
			return -1;
	}

	memcpy(thread[cur].env, thread[cur].env2, sizeof(thread[cur].env2));
	thread[cur].valid = 1;
	thread[cur].f = f;
	thread[cur].p = p;

	addTail(cur);
	return cur;

}

/*   	MyYieldThread (t) causes the running thread, call it T, to yield to
 * 	thread t. Returns the ID of the thread that yielded to the calling
 *  	thread T, or -1 if t is an invalid ID.  Example: given two threads
 *  	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 * 	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 * 	will resume by returning from its call to MyYieldThread (2), which
 *  	will return the value 2.
 */

int MyYieldThread (t)
	int t;				// thread being yielded to
{
	if (! MyInitThreadsCalled) {
		Printf ("MyYieldThread: Must call MyInitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("MyYieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("MyYieldThread: Thread %d does not exist\n", t);
		return (-1);
	}

    int cur = head;
	delete(cur);
	addTail(cur);
	delete(t);
	addHead(t);

	cur = MyGetThread();
	if (cur == t) {
		return cur;
	}
	if (setjmp(thread[cur].env) == 0) {
		preThread = cur;
		curThread = t;
		longjmp(thread[t].env, t + 1);
	}
	return preThread;
}

/*   	MyGetThread () returns ID of currently running thread. 
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyGetThread: Must call MyInitThreads first\n");
		Exit ();
	}
	return curThread;
}

/* 	MySchedThread () causes the running thread to simply give up the
 *  	CPU and allow another thread to be scheduled.  Selecting which
 *  	thread to run is determined here.  Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads). 
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MySchedThread: Must call MyInitThreads first\n");
		Exit ();
	}
	if (size == 1)
		MyYieldThread(head);
	else
		MyYieldThread(queue[head].next);
}

/* 	MyExitThread () causes the currently running thread to exit. 
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyExitThread: Must call MyInitThreads first\n");
		Exit ();
	}
	int cur = MyGetThread();
	thread[cur].valid = 0;
	delete(cur);
	if (size == 0)
		Exit();
	else 
		MyYieldThread(head);
}
