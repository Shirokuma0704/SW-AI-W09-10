/* file.c: Implementation of memory backed file object (mmaped object). */

#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "vm/vm.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* Do the munmap */
/* addr에서 시작하는 매핑 하나를 통째로 풀어요.
 * addr는 mmap이 돌려준 주소라서 항상 매핑의 첫 페이지 va예요.
 * 매핑이 몇 장인지는 첫 페이지 aux의 page_cnt에만 적혀 있어요. */
void
do_munmap (void *addr) {
	struct thread *curr = thread_current();
	struct page *p = va_to_page(addr);
	if (p == NULL) goto err;

	int page_cnt;
	struct aux *aux;

	/* aux가 어디 있는지는 첫 페이지의 "지금" 상태에 따라 달라요.
	 * page_get_type은 uninit이어도 초기화된 뒤의 타입(VM_FILE)을 돌려줘서 이 구별에는 못 써요.
	 * 그래서 지금 상태가 그대로 보이는 operations->type으로 갈라요. */
	switch (p->operations->type) {
	case VM_UNINIT:
		/* 아직 한 번도 안 읽은 페이지예요. union 자리에 uninit_page가 있어서 aux도 여기 있어요. */
		aux = p->uninit.aux;
		page_cnt = aux->page_cnt;
		break;

	case VM_FILE:
		/* 첫 폴트 때 file_backed_initializer가 aux를 file_page로 옮겨 둔 페이지예요. */
		page_cnt = p->file.aux->page_cnt;
		break;

	default:
		/* 코드·스택 같은 anon 페이지예요. mmap으로 만든 매핑이 아니에요. */
		goto err;
	}

	for (int n = 0; n != page_cnt; n++) {
		/* 매핑은 addr부터 연속된 페이지라서, n번째 va는 바로 계산돼요. */
		struct page *file_p = va_to_page(addr + (n * PGSIZE));
		/* NULL 검사가 먼저예요. ||는 왼쪽부터 계산해서, 순서가 바뀌면 NULL을 page_get_type에 넘겨요. */
		if (file_p == NULL || page_get_type(file_p) != VM_FILE ) goto err;

		/* 순서는 hash_delete → vm_dealloc_page → pml4_clear_page예요.
		 * hash_delete는 page 안의 hash_elem이 필요해서 page를 free하기 전에 해요. */
		if (hash_delete(&curr->spt.page_tables, &file_p->hash_elem) ==  NULL) goto err;

		/* 안에서 destroy가 불려요. file 페이지면 destroy가 pml4의 dirty bit를 읽으니까
		 * pml4는 아직 지우면 안 돼요. */
		vm_dealloc_page(file_p);

		/* SPT와 pml4 둘 다에서 지워야, 다시 접근했을 때 폴트가 나고 되살아나지도 않아요.
		 * file_p는 이미 free됐어서, va는 page가 아니라 addr에서 다시 계산해요.
		 * 한 번도 안 읽은 페이지는 pml4에 없지만, pml4_clear_page는 그래도 괜찮아요. */
		pml4_clear_page(curr->pml4, addr + (n * PGSIZE));
	}
	return;

	/* TODO: 예외 처리는 자리만 만들어 뒀어요. 검사는 munmap syscall에서 거르기로 했고,
	 * PANIC은 커널 전체를 멈추니까 나중에 바꿔야 해요. */
	err:
	PANIC("hmm");
}
