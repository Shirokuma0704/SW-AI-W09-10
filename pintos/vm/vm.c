/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "kernel/hash.h"
#include "threads/mmu.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include <string.h>

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
uint64_t va_to_hash (const struct hash_elem *e, void *aux UNUSED);
bool page_less (const struct hash_elem *a,const struct hash_elem *b, void *aux UNUSED);

void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);
// struct hash_elem *vm_to_hash_elem(struct supplemental_page_table *spt, void *va);
//struct page *vm_entry(struct supplemental_page_table *spt, void *va);

/* initializer와 함께 대기 중인 페이지 객체를 만들어요. 페이지를 만들고 싶다면,
 * 직접 만들지 말고 이 함수나 `vm_alloc_page`를 통해 만드세요. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
	   vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* upage가 이미 차지되어 있는지 아닌지 확인해요. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: 페이지를 만들고, VM 타입에 따라 initializer를 가져오고,
		 * TODO: 그다음 uninit_new를 불러서 "uninit" 페이지 구조체를 만들어요.
		 * TODO: uninit_new를 부른 뒤에 필드를 수정해야 해요. */
		/* 페이지 껍데기만 만들어요. 내용은 첫 페이지 폴트 때 채워요. */
		struct page *p = malloc(sizeof(struct page));
		if (p == NULL) goto err;

		bool (*initializer)(struct page *, enum vm_type, void *);

		/* 타입에 맞는 initializer를 골라요. 첫 폴트 때 uninit_initialize가 이걸 불러서
		 * 페이지를 anon / file 페이지로 바꿔요.
		 * case마다 break가 있어야 해요. 없으면 아래 case를 지나 default의 err까지 흘러내려 가요. */
		switch (VM_TYPE(type)) {
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
			default:
				/* 위에서 malloc한 p를 돌려주고 나가요. 안 그러면 아무도 가리키지 않는 채 남아요. */
				free(p);
				goto err;
		}

		/* 두 번째 인자가 이 페이지의 va가 돼요. SPT는 이 va로 페이지를 찾으니까 유저 주소 upage를 넣어요.
		 * (&p를 넣으면 지역 변수 p의 커널 스택 주소가 들어가서, 모든 페이지의 va가 같아져요.) */
		/* 페이지의 기본 상태와 초기화에 필요한 정보를 설정한다. */
		uninit_new(p, upage, init, type, aux, initializer);

		/* TODO: 페이지를 spt에 넣어요. */
		/* 요청받은 페이지의 쓰기 권한을 저장한다. */
		p->writable = writable;

		/* 같은 va가 이미 있어서 못 넣으면 실패로 처리해요. */
		if (spt_insert_page(spt, p) == false) {
			free(p);	// 넣지 못한 p도 돌려줘요
			goto err;
		}
		/* 새로 등록했을 때만 성공이에요. */
		return true;
	}
	/* upage가 이미 차지되어 있었거나, 위에서 실패했으면 여기로 와서 false예요. */
	err:
		return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = malloc(sizeof(struct page));

	/* TODO: Fill this function. */
	/* 찾기용 임시 page예요. 해시(va_to_hash)와 비교(page_less)가 va만 보니까 va만 채우면 돼요. */
	page->va = va;
	/* free하기 전에 결과를 받아 둬요. free한 page는 다시 건드리면 안 돼요. */
	struct hash_elem *result = hash_find(&spt->page_tables, &page->hash_elem);
	if (result == NULL) {
		free(page);
		return NULL;
	}
	free(page);
	return hash_entry(result, struct page, hash_elem);
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	/* 같은 va가 이미 있으면 넣지 않아요. 두 번째 인자는 va 값 그 자체예요 (&page->va가 아니라 page->va). */
	if (spt_find_page(spt, page->va) == NULL) {
		/* hash_insert는 넣는 데 성공하면 NULL을, 같은 원소가 이미 있으면 그 원소를 돌려줘요 (hash.c 주석). */
		struct hash_elem *result = hash_insert(&spt->page_tables, &page->hash_elem);
		if (result == NULL) succ = true;
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* struct frame은 커널이 들고 다니는 관리 카드예요. 몇십 바이트라서 malloc으로 받아요.
	 * (palloc으로 받으면 4KB를 통째로 쓰고, 유저 풀 칸도 하나 줄어요.) */
	struct frame *frame = malloc(sizeof(struct frame));
	if (frame == NULL)
	{
		goto err;
	}

	/* TODO: Fill this function. */
	/* 실제 4KB 칸은 유저 풀에서 따로 받아서 kva에 담아요. 커널은 이 주소로 그 칸을 읽고 써요. */
	frame->kva = palloc_get_page(PAL_USER);
	/* malloc은 받은 메모리를 비워 주지 않아요. 아직 연결된 페이지가 없다는 걸 직접 적어 둬요. */
	frame->page = NULL;
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;

	err:
		ASSERT("todo");
		return NULL;
}

/* 스택 확장 조건을 통과한 주소에 새 페이지를 등록한다.
 * addr는 호출자가 페이지 시작 주소로 정렬하여 전달한다.
 * 실제 프레임 확보와 매핑은 호출자 쪽 claim 경로에서 수행한다. */
static void
vm_stack_growth (void *addr UNUSED) {
    /* 익명 페이지이며 스택이라는 표시를 붙이고, 쓰기를 허용한다.
     * 등록에 실패하면 여기서 돌아간다.
     * 호출자는 SPT를 재조회하여 등록 성공 여부를 확인한다. */
	if (vm_alloc_page(VM_ANON | VM_MARKER_0, addr, true) == false)
		return;
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* 폴트를 복구하면 true, 복구할 수 없으면 false를 반환한다. */
bool
vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                    bool user UNUSED, bool write UNUSED,
                    bool not_present UNUSED)
{
    struct supplemental_page_table *spt = &thread_current()->spt;
    struct page *page = NULL;

    /* SPT 검색·페이지 등록에는 정렬된 주소를 사용한다. */
    uintptr_t va = (uintptr_t)pg_round_down(addr);

    /* 스택 접근의 타당성은 실제 폴트가 난 원래 주소로 판단한다. */
    uintptr_t address = (uintptr_t)addr;

    /* USER_STACK을 기준으로 스택을 최대 1MiB까지 허용한다. */
    uintptr_t mb = (1 << 20);

    uintptr_t rsp;

    /* 사용자 모드 폴트에서는 프레임의 사용자 rsp를 사용한다.
     * 커널 모드에서 사용자 메모리에 접근하다 발생한 폴트라면,
     * 시스템 콜 진입 때 저장한 사용자 rsp를 사용한다. */
    if (user == true)
        rsp = f->rsp;
    else
        rsp = thread_current()->user_rsp;

    /* NULL과 커널 영역 주소는 사용자 페이지로 복구하지 않는다. */
    if (addr == NULL || va >= (uintptr_t)KERN_BASE)
        goto err;

    /* 현재 단계에서는 권한 위반 폴트를 복구하지 않는다.
     * 매핑이 없는 페이지에 대한 폴트만 다음 단계로 진행한다. */
    if (not_present == false)
        goto err;

    /* 첫 조회: 이미 등록된 페이지인지 확인한다. */
    page = spt_find_page(spt, (void *)va);

    if (page == NULL) {
        /* 등록된 페이지가 없으면 스택 확장 후보인지 검사한다.
         * - 사용자 rsp보다 최대 8바이트 아래까지 허용
         * - USER_STACK 아래 최대 1MiB 영역으로 제한
         * - USER_STACK 자체와 그 위는 제외
         *
         * 조건을 통과한 경우에만 새 스택 페이지를 등록한다. */
        if (rsp - 8 <= address &&
            USER_STACK - mb <= address &&
            address < USER_STACK) {
            vm_stack_growth((void *)va);
        }

        /* 재조회: 확장 함수가 돌아와도 지역변수 page는
         * 자동으로 바뀌지 않으므로 등록 결과를 다시 얻는다. */
        page = spt_find_page(spt, (void *)va);

        /* 확장이 거부됐거나 등록에 실패했다면 복구할 수 없다. */
        if (page == NULL)
            goto err;
    }

    /* 기존 페이지와 새 스택 페이지 모두 같은 권한 검사를 거친다. */
    if (write == true && page->writable != true)
        goto err;

    /* 확보한 page에 프레임을 연결하고 매핑·내용 초기화를 수행한다.
     * 그 작업의 성공 여부를 폴트 처리 결과로 반환한다. */
    return vm_do_claim_page(page);

err:
    /* 전체 자원 정리는 종료 경로에 맡기고 실패만 알린다. */
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	struct thread *curr = thread_current();

	/* TODO: Fill this function */
	page = spt_find_page(&curr->spt, va);

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	struct thread *curr = thread_current();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 인자 순서는 (pml4, 유저 가상 주소, 커널 주소, 쓰기 가능)이에요. 이 페이지의 va를 프레임의 kva에 이어 줘요.
	 * page->writable을 넘겨서 읽기 전용 페이지(코드 등)엔 쓰기를 막아요. */
	bool result = pml4_set_page(
		curr->pml4, page->va, frame->kva, page->writable
	);

	/* 주소 연결에 실패했다면 false를 반환하고 종료한다. */
	if (result == false)
		return result;

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/* 스레드마다 SPT 해시 테이블을 하나씩 만들어요. 해시는 va_to_hash, 순서 비교는 page_less로 해요. */
	hash_init(&spt->page_tables, va_to_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	/* __do_fork 안에서 자식 스레드가 불러요. 그래서 curr는 자식이고, src가 부모 SPT예요. */
	struct thread *curr = thread_current();
	struct thread *parents = curr->parents;

	/* 1단계: 부모 SPT를 한 바퀴 돌면서 va를 모아 둬요. */
	size_t hash_count = hash_size(&src->page_tables);
	uintptr_t *va[hash_count];

	struct hash_iterator i;
	hash_first (&i, &src->page_tables);

	int n = 0;
	while (hash_next (&i)) {
		struct page *p = hash_entry (hash_cur (&i), struct page, hash_elem);
		va[n] = p->va;
		n++;
	}

	/* 2단계: 모아 둔 va로 부모 page를 하나씩 찾아서 자식 page를 만들어요. */
	for (size_t x = 0; x != hash_count; x++) {
		/* 자식 page로 쓸 구조체예요. 먼저 va만 채워서 부모 SPT를 찾는 검색 키로 써요. */
		struct page *insert_page = malloc(sizeof(struct page));
		insert_page->va = va[x];
		struct page *p = hash_entry (hash_find(&src->page_tables, &insert_page->hash_elem), struct page, hash_elem);

		/* 부모 page를 통째로 복사해요. va, writable, operations는 그대로 쓰고,
		 * aux와 frame은 아래 switch에서 자식 것으로 바꿔요. */
		insert_page = memcpy(insert_page, p, sizeof(struct page));

		switch (VM_TYPE(p->operations->type)) {

			/* 아직 첫 폴트가 안 난 페이지예요. 내용은 자식이 첫 폴트 때 lazy_load_segment로 읽어요.
			 * aux는 자식 몫을 새로 받아요. 부모 aux를 같이 가리키면 두 쪽이 각자 첫 폴트 때 같은 aux를 free해요.
			 * 파일은 자식의 running_file을 빌려요. 부모 것은 부모가 먼저 끝나면 process_exit에서 닫혀요. */
			case VM_UNINIT:
				insert_page->uninit.aux = malloc(sizeof(struct aux));

				memcpy(insert_page->uninit.aux, p->uninit.aux, sizeof(struct aux));

				struct aux *aux = insert_page->uninit.aux;
				aux->file = curr->running_file;
				break;

			/* 이미 내용이 올라온 페이지예요. 자식 프레임을 새로 받고, 부모 칸 내용을 복사하고, 자식 pml4에 매핑해요.
			 * vm_do_claim_page를 쓰지 않는 건 그 끝의 swap_in이 내용을 스왑에서 채우려 하기 때문이에요. 여기선 memcpy가 채워요.
			 * frame을 자식 것으로 바꾼 뒤에 memcpy해요. 통째 복사 직후의 insert_page->frame은 아직 부모 frame이에요.
			 * 부모 페이지가 스왑으로 나가 있어서 frame이 없는 경우는 6절에서 다뤄요. */
			case VM_ANON:
				struct frame *fa = vm_get_frame();
				if (fa == NULL)
					PANIC("TODO");

				insert_page->frame = fa;
				fa->page = insert_page;

				memcpy(insert_page->frame->kva, p->frame->kva, PGSIZE);

				if (pml4_set_page(curr->pml4, insert_page->va, fa->kva, insert_page->writable) == false)
					return false;

				break;

			/* mmap(5절)이 아직 없어서 지금은 VM_FILE 페이지가 생기지 않아요. ANON과 같은 방식으로 두었고,
			 * 내용을 파일에서 다시 읽을지, 파일을 빌릴지 복제할지 같은 건 5절에서 다시 정해요. */
			case VM_FILE:
				struct frame *ff = vm_get_frame();
				if (ff == NULL)
					PANIC("TODO");

				insert_page->frame = ff;
				ff->page = insert_page;

				memcpy(insert_page->frame->kva, p->frame->kva, PGSIZE);

				if (pml4_set_page(curr->pml4, insert_page->va, ff->kva, insert_page->writable) == false)
					return false;
				break;

			default:
				return false;
		}

		spt_insert_page(dst, insert_page);
	}
	return true;

}

/* hash_destroy()가 해시 원소마다 호출하는 정리 콜백이다. */
static void
page_destroy(struct hash_elem *e, void *aux UNUSED) {

	/* 해시 원소의 주소로, 그것을 포함한 page의 주소를 구한다. */
	struct page *page = hash_entry(e, struct page, hash_elem);

	/* 페이지의 자원을 정리하고 page 구조체를 해체한다. */
	vm_dealloc_page(page);
}

/* SPT에 등록된 모든 페이지와 해시 테이블의 버킷 공간을 정리한다. */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* hash_destroy()가 원소마다 page_destroy()를 호출하고,
	 * 마지막으로 해시 테이블의 버킷 공간을 해제한다. */
	hash_destroy(&spt->page_tables, page_destroy);
}

/* SPT 해시 함수예요. va 값만 해시해요.
 * page_less도 va만 비교하니까, 둘이 같은 기준이어야 같은 va가 같은 버킷에서 찾아져요.
 * hash_bytes의 첫 인자는 "바이트가 놓인 곳"이라서 va의 주소인 &p->va를 넘겨요. */
uint64_t va_to_hash (const struct hash_elem *e, void *aux UNUSED) {
	const struct page *p = hash_entry(e, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof(p->va));
}

/* SPT 비교 함수예요. va가 작은 쪽이 앞이에요.
 * 어느 쪽도 작지 않으면(va가 같으면) 해시 테이블은 같은 원소로 봐요. */
bool page_less (const struct hash_elem *a,const struct hash_elem *b, void *aux UNUSED) {
	const struct page *ap = hash_entry(a, struct page, hash_elem);
	const struct page *bp = hash_entry(b, struct page, hash_elem);

	if (ap->va >= bp->va) return false;
	return true;
}

/* 현재 스레드의 SPT에서 va에 해당하는 page를 찾아요. 없으면 NULL이에요.
 * va는 페이지 경계에 맞는 값이어야 해요. 해시가 va를 그대로 비교해서, 경계에서 어긋나면 못 찾아요. */
struct page*
va_to_page(void *va) {
	struct thread *curr = thread_current();
	/* 찾기용 임시 page예요. 해시와 비교 함수가 va만 보니까 va만 채워요. */
	struct page *p = malloc(sizeof(struct page));
	p->va = va;

	struct hash_elem *elem = hash_find(&curr->spt.page_tables, &p->hash_elem);
	/* 찾았든 못 찾았든 임시 page는 여기서 free해요. return보다 먼저 해야 두 경로 다 새지 않아요. */
	free(p);

	if (elem == NULL) return NULL;

	return hash_entry(elem, struct page, hash_elem);
}

// struct hash_elem *
	// vm_to_hash_elem(struct supplemental_page_table *spt, void *va) {
	// struct hash_iterator i;
	// bool find = false;
	// struct page *f = NULL;

	// hash_first (&i, &spt->page_tables);
	// while (hash_next (&i)) {
		// f = hash_entry (hash_cur (&i), struct page, hash_elem);
		// if (va == f->va) {
			// find = true;
			// break;
		// }
	// }

	// if (!find) return NULL;
	// return &f->hash_elem;
// }

// struct page *vm_entry (struct supplemental_page_table *spt, void *va) {
// 		struct hash_iterator i;
// 		bool find = false;
// 		struct page *f = NULL;
//
// 		hash_first (&i, &spt->page_tables);
// 		while (hash_next (&i)) {
// 			f = hash_entry (hash_cur (&i), struct page, hash_elem);
// 			if (va == f->va) {
// 				find = true;
// 				break;
// 			}
// 		}
//
// 		if (!find) return NULL;
// 		return f;
// 	}