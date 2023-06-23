/* vm.c: Generic interface for virtual memory objects. */
#include "threads/malloc.h"
#include "threads/mmu.h"
#include "threads/thread.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
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

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
/* initializer를 사용하여 보류 중인(pending) 페이지 객체를 생성합니다. 페이지를 생성하려면
직접 생성하지 말고 이 함수 또는 `vm_alloc_page`를 통해 생성하세요. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* upage가 이미 사용 중인지 아닌지 확인합니다. */
	if (spt_find_page (spt, upage) == NULL) {
		// 페이지를 생성하고, VM 타입에 따라 적절한 초기화 함수를 가져온다.
		struct page *page = malloc(sizeof(struct page));
		page->va = upage;

		bool (*page_initializer)(struct page *, enum vm_type, void *);

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			page_initializer = anon_initializer;
			break;
		case VM_FILE:
			page_initializer = file_backed_initializer
			break;
		}

		// 초기화 함수를 인자로 갖는 uninit_new를 호출하여 uninit 페이지 구조체를 생성한다.
		uninit_new(page, page->va, init, type, aux, page_initializer)
		
		// uninit_new를 호출한 후 필드를 수정해야 한다.

		// 페이지를 spt에 삽입한다.
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* 보조페이지 테이블에서로부터 가상 주소(va)와 대응되는 페이지 구조체를 찾아서 반환한다. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */

	page = malloc(sizeof(struct page));
	page->va = pg_round_down(va);
	struct hash_elem *e = hash_find(&spt->hash_table, &page->hash_elem);
	free(page);

	return e == NULL ? NULL : hash_entry(e, struct page, hash_elem);
}

/* Insert PAGE into spt with validation. */
/* 보조 페이지 테이블에 페이지 구조체를 삽입한다. 보조 테이블에서 가상 주소가 존재하지 않는지 검사해야 한다. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* TODO: Fill this function. */
	return hash_insert(&spt->hash_table, &page->hash_elem) == NULL ? true : false;
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

/* palloc()을 사용하여 프레임을 가져옵니다. 사용 가능한 페이지가 없으면 페이지를
 * 추방하고 반환합니다. 이 함수는 항상 유효한 주소를 반환합니다. 즉, 사용자 풀
 * 메모리가 가득 찬 경우, 이 함수는 사용 가능한 메모리 공간을 얻기 위해 프레임을
 * 추방합니다. */
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	void *pa = palloc_get_page(PAL_USER); // 가용한 물리 주소를 가져온다.
	if (pa == NULL) {
		PANIC("todo");
	}

	frame = malloc(sizeof(struct frame));
	frame->kva = pa;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
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

	return vm_do_claim_page (page);
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
	/* TODO: Fill this function */
	// 페이지를 얻어야 한다.
	page = spt_find_page(&thread_current()->spt, va);
	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* 페이지를 요청하고 MMU를 설정합니다. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* TODO: 페이지의 VA를 프레임의 PA에 매핑하는 페이지 테이블 항목을 삽입하세요. */
	struct thread *curr_thread = thread_current();
	bool result = pml4_set_page(curr_thread->pml4, page, frame, is_writable(curr_thread->pml4));

	return result ? swap_in (page, frame->kva) : false
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(spt, page_hash, page_less, NULL);
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


/* 페이지의 가상 주소를 해싱하는 함수 */
uint64_t
page_hash (const struct hash_elem *e, void *aux) {
	// 1. 해당 hash_elem 요소를 갖고 있는 page를 가져온다.
	struct page *p = hash_entry(e, struct page, hash_elem);
	// 2. 페이지의 가상 주소값을 hash_bytes 함수를 이용해 해싱한다.
	return hash_bytes(&p->va, sizeof(p->va));
}

/* 페이지의 키 값을 비교 하는 함수 */
bool
page_less (const struct hash_elem *a,
	const struct hash_elem *b,	void *aux) {			
		// 1. hash_entry로 page 구조체를 가져온다.
		struct page *p1 = hash_entry(a, struct page, hash_elem);
		struct page *p2 = hash_entry(b, struct page, hash_elem);
		// 2. generate_hash 함수로 키를 생성하고 비교한다.
		return p1->va < p2->va;
	}
