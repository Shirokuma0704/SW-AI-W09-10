#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* 해시 테이블.
 *
 * 이 자료구조는 Project 3용 Tour of Pintos에 자세히 문서화되어
 * 있어요.
 *
 * 이건 체이닝을 쓰는 표준 해시 테이블이에요. 테이블에서 원소 하나를
 * 찾으려면, 원소의 데이터에 대해 해시 함수를 계산하고 그 값을
 * 이중 연결 리스트들의 배열에 대한 인덱스로 쓴 다음, 그 리스트를
 * 선형으로 탐색해요.
 *
 * 체인 리스트는 동적 할당을 쓰지 않아요. 대신, 해시 안에 들어갈
 * 가능성이 있는 각 구조체는 struct hash_elem 멤버를 내장해야 해요.
 * 모든 해시 함수는 이 `struct hash_elem'들을 대상으로 동작해요.
 * hash_entry 매크로는 struct hash_elem에서 그것을 담고 있는 구조체
 * 객체로 거꾸로 변환할 수 있게 해 줘요. 이건 연결 리스트 구현에서
 * 쓰인 것과 같은 기법이에요. 자세한 설명은 lib/kernel/list.h를
 * 참고하세요. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

/* 해시 원소. */
struct hash_elem {
	struct list_elem list_elem;
};

/* 해시 원소 HASH_ELEM을 가리키는 포인터를, HASH_ELEM이 내장되어 있는
 * 구조체를 가리키는 포인터로 변환해요. 바깥 구조체의 이름 STRUCT와
 * 해시 원소의 멤버 이름 MEMBER를 넘겨 주세요. 예시는 파일 맨 위의
 * 큰 주석을 보세요. */
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* 보조 데이터 AUX가 주어졌을 때, 해시 원소 E의 해시 값을 계산해서
 * 반환해요. */
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* 보조 데이터 AUX가 주어졌을 때, 두 해시 원소 A와 B의 값을 비교해요.
 * A가 B보다 작으면 true를, A가 B보다 크거나 같으면 false를
 * 반환해요. */
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* 보조 데이터 AUX가 주어졌을 때, 해시 원소 E에 대해 어떤 동작을
 * 수행해요. */
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* 해시 테이블. */
struct hash {
	size_t elem_cnt;            /* 테이블 안의 원소 개수. */
	size_t bucket_cnt;          /* 버킷 개수, 2의 거듭제곱. */
	struct list *buckets;       /* `bucket_cnt'개 리스트의 배열. */
	hash_hash_func *hash;       /* 해시 함수. */
	hash_less_func *less;       /* 비교 함수. */
	void *aux;                  /* `hash'와 `less'를 위한 보조 데이터. */
};

/* 해시 테이블 반복자. */
struct hash_iterator {
	struct hash *hash;          /* 해시 테이블. */
	struct list *bucket;        /* 현재 버킷. */
	struct hash_elem *elem;     /* 현재 버킷 안의 현재 해시 원소. */
};

/* 기본 생애 주기. */
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);
void hash_clear (struct hash *, hash_action_func *);
void hash_destroy (struct hash *, hash_action_func *);

/* 검색, 삽입, 삭제. */
struct hash_elem *hash_insert (struct hash *, struct hash_elem *);
struct hash_elem *hash_replace (struct hash *, struct hash_elem *);
struct hash_elem *hash_find (struct hash *, struct hash_elem *);
struct hash_elem *hash_delete (struct hash *, struct hash_elem *);

/* 순회. */
void hash_apply (struct hash *, hash_action_func *);
void hash_first (struct hash_iterator *, struct hash *);
struct hash_elem *hash_next (struct hash_iterator *);
struct hash_elem *hash_cur (struct hash_iterator *);

/* 정보. */
size_t hash_size (struct hash *);
bool hash_empty (struct hash *);

/* 예시 해시 함수들. */
uint64_t hash_bytes (const void *, size_t);
uint64_t hash_string (const char *);
uint64_t hash_int (int);

#endif /* lib/kernel/hash.h */
