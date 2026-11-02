#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* 커널 스레드 또는 사용자 프로세스.
 *
 * 각 스레드 구조체는 자신만의 4KB 페이지에 저장된다.
 * 스레드 구조체 자체는 페이지의 가장 아래쪽(offset 0)에 위치한다.
 * 페이지의 나머지 공간은 커널 스택을 위해 예약되어 있으며,
 * 커널 스택은 페이지의 가장 위쪽(offset 4KB)에서 아래 방향으로 성장한다.
 *
 * 다음은 이를 그림으로 나타낸 것이다.
 *
 *      4 kB +---------------------------------+
 *           |          커널 스택             	 |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         아래 방향으로 성장          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * 여기서 중요한 점은 두 가지다.
 *
 *    1. 첫째, struct thread의 크기가 지나치게 커져서는 안 된다.
 *       크기가 너무 커지면 커널 스택을 위한 공간이 부족해진다.
 *       기본 struct thread의 크기는 몇 바이트 정도로 작다.
 *       따라서 1KB보다 훨씬 작은 크기를 유지하는 것이 좋다.
 *
 *    2. 둘째, 커널 스택도 지나치게 커져서는 안 된다.
 *       스택이 넘치면 스레드의 상태 정보가 손상될 수 있다.
 *       따라서 커널 함수에서는 큰 구조체나 배열을 일반 지역 변수로
 *       할당해서는 안 된다.
 *       대신 malloc()이나 palloc_get_page()를 사용해 동적으로 할당해야 한다.
 *
 * 이 두 문제 중 하나가 발생하면 가장 먼저 나타나는 증상은
 * thread_current()에서의 assert 실패일 가능성이 높다.
 * thread_current()는 실행 중인 스레드의 struct thread에 있는
 * magic 멤버가 THREAD_MAGIC으로 설정되어 있는지 검사한다.
 * 스택 오버플로가 발생하면 일반적으로 이 값이 변경되어
 * assert가 실패하게 된다.
 */

/* elem 멤버는 두 가지 용도로 사용된다.
 * 
 * thread.c에서는 실행 대기 큐(run queue)의 원소로 사용되고,
 * synch.c에서는 세마포어 대기 목록(semaphore wait list)의 원소로 사용된다.
 *
 * 이처럼 두 가지 용도로 사용할 수 있는 이유는
 * 두 상태가 서로 배타적이기 때문이다.
 *
 * READY 상태의 스레드만 실행 대기 큐에 들어가고,
 * BLOCKED 상태의 스레드만 세마포어 대기 목록에 들어간다.
 */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */
	int64_t wakeup_tick;

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

void thread_sleep (int64_t ticks);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
