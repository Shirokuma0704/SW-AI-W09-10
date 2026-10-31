#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

/* 자식 하나의 종료 기록이에요 (#113).
   부모가 malloc해서 자기 child_record_list에 넣고, wait나 부모 exit에서 free해요.
   자식은 끝날 때 status를 쓰고 child_address를 NULL로 바꾸는 것만 해요. */
struct record
{
    tid_t tid;                      // 자식의 tid. wait(tid)로 찾을 때 써요
    int status;                     // 자식의 종료 코드
    struct thread *child_address;   // 자식 struct thread 주소. NULL이면 자식이 이미 끝난 거예요
    struct semaphore process_sema;  // 부모는 wait에서 down, 자식은 exit에서 up
    struct list_elem child_elem;    // 부모의 child_record_list에 매달리는 원소
};

/* fork할 때 부모가 자식(__do_fork)에게 건네는 인자 묶음이에요 (#111).
   thread_create의 aux 자리는 포인터 하나뿐이라 둘을 묶어서 보내요.
   본체는 process_fork의 지역 변수예요. 부모가 자식의 복제가 끝날 때까지 기다리니까 그동안은 살아 있어요. */
struct fork_args
{
    struct thread *parents;         // fork를 부른 부모 스레드
    struct intr_frame *if_;         // 부모가 syscall로 들어올 때 쌓인 유저 문맥. parent->tf에는 이게 없어서 따로 넘겨요
};

/* load_segment가 페이지마다 하나씩 만들어서 lazy_load_segment에 넘기는 정보 묶음이에요.
   페이지를 바로 읽지 않고 첫 폴트 때 읽으니까, 그때 "파일 어디서 몇 바이트"를 알 수 있게 미리 적어 둬요.
   load_segment 안에 있던 정의를 헤더로 옮겼어요. lazy_load_segment에서도 같은 모양을 봐야 해서요. */
struct aux {
    struct file *file;              // 읽어 올 실행 파일
    uint32_t read_bytes;            // 이 페이지에서 파일로 채울 바이트 수
    uint32_t zero_bytes;            // 나머지를 0으로 채울 바이트 수 (read_bytes + zero_bytes = PGSIZE)
    off_t ofs;                      // 파일 안에서 읽기 시작할 위치
    uint8_t *upage;                 // 이 페이지의 유저 가상 주소
    bool writable;                  // 유저가 이 페이지에 쓸 수 있는지
};


#endif /* userprog/process.h */
