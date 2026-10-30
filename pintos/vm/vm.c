/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "kernel/hash.h"
#include "threads/mmu.h"
#include "threads/thread.h"

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
		uninit_new(p,upage, init, type, aux, initializer);

		/* TODO: 페이지를 spt에 넣어요. */
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

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/* 폴트 주소는 페이지 중간일 수도 있어요. SPT 키는 페이지 시작 주소라서,
	 * 아래 12비트를 지워(pg_round_down) 시작 주소로 내린 뒤에 찾아요. 예: 0x400a3c → 0x400000 */
	uintptr_t *va = pg_round_down(addr);
	/* 스택 크기 한도예요. 이 프로젝트는 스택을 최대 1MB까지만 키워요 (위키 4절 "스택 크기 한도"). */
	uintptr_t mb = (1 << 20);

	/* 폴트 검증이에요. 아직 작성 중이에요.
	 * 순서대로 ① 유저 폴트가 커널 주소를 건드렸는지, ② SPT에 없는 주소인지 (스택 확장 후보 자리),
	 * ③ 읽기 전용 페이지에 쓰려고 했는지를 봐요. 걸리면 err로 가요. */
	if (user == true && *va < (uintptr_t) KERN_BASE) goto err;

	page = spt_find_page(spt, va);

	if (page == NULL)
	{
		if (f->rsp - PGSIZE > *va && f->rsp - PGSIZE - mb > *va)
			ASSERT("todo");
		goto err;
	}

	if (write == true && page->writable != true) {
		free(page);
		goto err;
	}
	return vm_do_claim_page (page);

	/* 검증에 걸린 폴트가 오는 곳이에요. 여기서 어떻게 처리할지는 아직 정하는 중이에요. */
	err:
	// kill?
	ASSERT("INVALIDATE")
	return NULL;
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
	bool result = pml4_set_page(curr->pml4,page->va,frame->kva, page->writable);
	if (result == true) return result;

	return swap_in (page, frame->kva);
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
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
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