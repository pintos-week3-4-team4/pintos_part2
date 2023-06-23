#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include "hash.h"

enum vm_type {
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */
	struct hash_elem hash_elem; // supplemental_page_table에 정의된 해시 테이블의 요소이다.

	/* Your implementation */

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* The representation of "frame" */
struct frame {
	void *kva; // 커널 가상 주소
	struct page *page;
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
/* 페이지의 파일 포인터, 오프셋, 크기를 저장 
 * 프로그램 탑재 시 가상 주소공간의 각 페이지에 vm_entry를 할당함.
 * 페이지 폴트 발생 시, 가상 주소에 해당하는 vm_entry를 탐색
 * vm_entry가 없는 경우: segmentation fault 발생 시키며 종료
 * vm_enry가 존재할 경우: vm_entry에 있는 파일 포인터, 읽기 시작할 오프셋,
 * 읽어야 할 크기 등을 참조해서 물리 페이지를 할당하고 물리 메모리에 로드한 후, 물리 주소와 매핑 */
struct supplemental_page_table {
	struct hash *hash_table;

	// enum vm_type type; /* 페이지 타입 */
	// void *vaddr; /* 가상 페이지 주소*/
	// bool writable; /* 수정 가능 여부 플래그 */

	// bool is_loaded; /*물리 메모리 탑재 여부 플래그 */
	// struct *file file; /* 가상 주소와 매핑된 파일 */

	// /* Memory Mapped File에서 다룰 예정*/
	// struct list_elem mmap_elem; /* mmap 리스트 element*/

	// size_t offset; /* 읽어야 할 파일 오프셋 */
	// size_t read_bytes; /* 가상 페이지에 쓰여져 있는 데이터 크기 */
	// size_t zero_bytes; /* 0으로 채울 남은 페이지의 바이트 */

	// /* Swapping 과제에서 다룰 예정 */
	// size_t swap_slot; /* 스왑 슬롯 */

	// /* vm_entry들을 위한 자료구조 부분에서 다룰 예정 */
	// struct hash_elem elem; /* 해시 테이블 element */
};

#include "threads/thread.h"
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

uint64_t page_hash (const struct hash_elem *e, void *aux);
bool page_less (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

#endif  /* VM_VM_H */
