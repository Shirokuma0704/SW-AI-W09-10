/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

#include "vm/vm.h"
#include "vm/uninit.h"
#include "threads/malloc.h" /* 컴파일러가 free()의 선언을 알 수 있도록 넣습니다. */

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* Initalize the page on first fault */
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */
	return uninit->page_initializer(page, uninit->type, kva) &&
		   (init ? init (page, aux) : true);
}

/* uninit 페이지가 보유한 자원을 해제한다.
 * 대부분의 페이지는 초기화되면서 다른 페이지 타입으로 바뀌지만,
 * 실행 중 한 번도 접근하지 않은 페이지는 프로세스가 종료될 때까지
 * uninit 상태로 남을 수 있다.
 * page 구조체 자체는 호출자가 해제한다. */
static void
uninit_destroy (struct page *page)
{
	/* 이 페이지에 보관된 초기화 정보에 접근한다. */
	struct uninit_page *uninit = &page->uninit;

	/* 나중에 페이지를 초기화하려고 보관했던 정보를 해제한다. */
	if (uninit->aux != NULL)
		free(uninit->aux);
}
