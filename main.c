#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/syscall.h>

// Schedule Policy constant
#define FIFO 1
#define   RR 2
#define  SJF 3
#define PSJF 4

// RR quantum
#define Q 500

// CPU
#define MAINCPU 0
#define PROCCPU 1

// User-definde system call
#define PRINTK 436
#define  GETNS 437

void TIME_UNIT() {
	volatile unsigned long i;
	for (i = 0; i < 1000000UL; i++);
}

typedef struct {
	char N[32];
	unsigned R;
	unsigned T;
	unsigned T_left;
	pid_t pid;
	struct timespec start;
	struct timespec end;
	unsigned state;
} process;

int processCmp(const void *a, const void *b) {
	return (int *)(*(process *)a).R - (int *)(*(process *)b).R;
}

struct timespec getTimespec(unsigned long ns) {
	return (struct timespec){ns/1000000000UL, ns%1000000000UL};
}

void printProcess(process p) {
	struct timespec diff;
	int borrow = (p.end.tv_nsec < p.start.tv_nsec) ? 1 : 0;
	diff.tv_sec  = p.end.tv_sec - p.start.tv_sec - borrow;
	diff.tv_nsec = borrow * 1000000000 + p.end.tv_nsec - p.start.tv_nsec;
	printf("%s %u", p.N, p.pid);
#ifdef DEBUG
	printf(" %lu.%04lu %lu.%04lu %lu.%04lu",
		p.start.tv_sec % 100, p.start.tv_nsec / 100000,
		p.end  .tv_sec % 100, p.end  .tv_nsec / 100000,
		diff   .tv_sec, diff   .tv_nsec / 100000);
#endif
	printf("\n");
}

int setCPU(pid_t pid, int cpu) {
	if (cpu > sizeof(cpu_set_t)) {
		fprintf(stderr, "Invalid cpu.\n");
		return -1;
	}
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	return sched_setaffinity(pid, sizeof(mask), &mask);
}

int block(pid_t pid) {
	struct sched_param param;
	param.sched_priority = 0;
	return sched_setscheduler(pid, SCHED_IDLE, &param);
}

int execute(pid_t pid) {
	struct sched_param param;
	param.sched_priority = 0;
	return sched_setscheduler(pid, SCHED_OTHER, &param);
}

pid_t newProcess(process* p) {
	pid_t pid = fork();
	if (pid == -1) { // Error
		fprintf(stderr, "Fork error.\n");
		exit(-1);
	} else if (pid) { // parent
		return pid;
	} else { // child
		int ret;
		p->pid = getpid(); // set pid
		setCPU(p->pid, PROCCPU); // set CPU
		block(p->pid); // block first
		ret = syscall(GETNS, &(p->start)); // get start time
		for (unsigned i = 0; i < p->T; i++) {
			TIME_UNIT();
		}
		ret = syscall(GETNS, &(p->end)); // get end time
		// print to kernel
		char buf[256];
		sprintf(buf, "[project1] %d %lu.%09lu %lu.%09lu", p->pid,
			p->start.tv_sec, p->start.tv_nsec,
			p->end  .tv_sec, p->end  .tv_nsec);
		ret = syscall(PRINTK, buf, strlen(buf) + 1);
		exit(0);
	}
}

int findShortestJob(process* P, int n) {
	// find shortest T_left
	unsigned min_T = -1;
	int min = -1;
	int flag_endAll = 1;
	for (int i = 0; i < n; i++) {
		if (P[i].T_left) flag_endAll = 0;
		if (P[i].T_left && P[i].state && P[i].T_left < min_T) {
			min_T = P[i].T_left;
			min = i;
		}
	}
	if (flag_endAll) return -2; // all processes ended
	if (min >= 0) return min; // start new 
	return -1; // not yet started
}

int findNext(int cur, process* P, int n, int rr) {
	int flag_endAll = 1;
	for (int i = 0; i < n; i++) {
		// circular search if policy is RR
		int this = rr ? ((cur+i+1)%n) : i;
		if (P[this].T_left) {
			flag_endAll = 0;
			if (P[this].state) return this; // context switch 
			if (!rr) return -1; // FIFO: not yet started
			// RR: continue search
		}
	}
	if (flag_endAll) return -2; // all processes ended
	return -1; // not yet started
}

int nextProcess(int cur, process* P, int n, int policy, int *rrcount, int flag_newProcess) {
	// if not running / current ended / RR reached Q / 
	// policy is PSJF and there are new process coming
	if (cur < 0 || !P[cur].T_left || *rrcount>=Q || (policy == PSJF && flag_newProcess)) {
		// SJF / PSFJ : return shortest job
		if (policy == SJF || policy == PSJF) return findShortestJob(P, n);
		// FIFO / RR : find next job
		if (policy == RR) *rrcount = 0; // reset RR counter
		return findNext(cur, P, n, (policy == RR) ? 1 : 0);
	} else {
		if (policy == RR) *rrcount ++; // add RR counter
		return cur; // continue current process
	}
}

int main() {
#ifdef DEBUG
	// test syscall
	int ret = syscall(PRINTK, "HELLO KERNEL", 13);
	fprintf(stderr, "print_kernel returned %d\n", ret);
#endif

	// variables
	char S[5];
	int policy, n;

	// set policy
	scanf("%s", S);
	     if (!strcmp(S, "FIFO")) policy = FIFO;
	else if (!strcmp(S,   "RR")) policy =   RR;
	else if (!strcmp(S,  "SJF")) policy =  SJF;
	else if (!strcmp(S, "PSJF")) policy = PSJF;
	else {
		fprintf(stderr, "Invalid policy.\n");
		exit(-1);
	}

	// set processes
	scanf("%d", &n);
	process *P = (process *)malloc(sizeof(process) * n);
	for (int i = 0; i < n; i++) {
		scanf("%s %u %u\n", P[i].N, &(P[i].R), &(P[i].T));
		P[i].state = 0;
		P[i].T_left = P[i].T;
	}

	// sort by ready time, just in case
	qsort(P, n, sizeof(process), processCmp);

	// set cpu and priority
	setCPU(getpid(), MAINCPU);
	execute(getpid());

	// init
	int index = 0, cur = -1;
	process *curProcess = NULL;
	process *startNext = &(P[0]);
	unsigned curUnit = 0;
	int running = 1;
	int rrcount = 0;

	// run!
	while (1) {
		int flag_newProcess = 0;
		// start the next process if available
		while (startNext && curUnit >= startNext->R) {
			flag_newProcess = 1;
			startNext->pid = newProcess(startNext);
			startNext->state = 1;
			index ++;
			startNext = (index >= n) ? NULL : &(P[index]);
		}

		// check if finishing (optional)
		if (curProcess && !curProcess->T_left) waitpid(curProcess->pid, NULL, 0);

		// find next process on schedule and run
		int next = nextProcess(cur, P, n, policy, &rrcount, flag_newProcess);
		if (next == -2) break; // -2: all processes ended
		if (next >= 0) { // run a process
			if (next != cur) {
#ifdef DEBUG
				fprintf(stderr, "next = %d\n", next);
#endif
				if (curProcess) block(curProcess->pid);
				curProcess = &(P[next]);
				execute(curProcess->pid);
				cur = next;
			}
			curProcess->T_left --;
		} else { // -1: no process running
			if (curProcess) block(curProcess->pid);
			curProcess = NULL;
			cur = -1;
		}

		// run a time unit
		TIME_UNIT();
		curUnit ++;
	}

	for (int i = 0; i < n; i++) printProcess(P[i]);

#ifdef DEBUG
	fprintf(stderr, "All processes ended\n");
#endif

	return 0;
}