/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/vaddr.h" /* PGSIZE 정의 */
#include <string.h> /* memset() 선언 */

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = NULL;
}

/* 확보한 프레임을 새 익명 페이지로 사용할 수 있도록 초기화한다. */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* 이전 내용이나 디버깅용 값이 남지 않도록 한 페이지 전체를 0으로 채운다. */
	memset(kva, 0, PGSIZE);

	/* 같은 page 구조체가 익명 페이지용 연산을 사용하도록 설정한다. */
	page->operations = &anon_ops;

	/* 익명 페이지 전용 관리 정보의 위치를 얻는다. */
	struct anon_page *anon_page = &page->anon;

	/* 초기화 성공을 호출자에게 알린다. */
	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
