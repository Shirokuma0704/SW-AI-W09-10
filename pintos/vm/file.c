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
void
do_munmap (void *addr) {
	struct thread *curr = thread_current();
	struct page *p = va_to_page(addr);
	if (p == NULL) goto err;

	int page_cnt;
	struct aux *aux;

	switch (p->operations->type) {
	case VM_UNINIT:
		aux = p->uninit.aux;
		page_cnt = aux->page_cnt;
		break;

	case VM_FILE:
		page_cnt = p->file.aux->page_cnt;
		break;

	default:
		goto err;
	}

	for (int n = 0; n != page_cnt; n++) {
		struct page *file_p = va_to_page(addr + (n * PGSIZE));
		if (file_p == NULL || page_get_type(file_p) != VM_FILE ) goto err;

		if (hash_delete(&curr->spt.page_tables, &file_p->hash_elem) ==  NULL) goto err;

		vm_dealloc_page(file_p);

		pml4_clear_page(curr->pml4, addr + (n * PGSIZE));
	}
	return;

	err:
	PANIC("hmm");
}
