/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

//pg_round_donw() 함수를 위해 추가
#include "threads/mmu.h"

//vm_entry를 위해 추가
#include "userprog/process.h"

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
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) { // initializer는 초기화하는 함수, upage는 va

	ASSERT (VM_TYPE(type) != VM_UNINIT) // VM_UNINIT(초기화되지 않은 페이지) 자체가 지연로딩(lazy_loading)을 위해서 존재하는 타입

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) { // spt안에 va가 없으면(즉, 할당할 수 있는 상태면)
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		// 페이지를 생성한다.
		struct page *p = (struct page *)malloc(sizeof(struct page));
		// if(p == NULL){
		// 	goto err;
		// }
		// 함수 포인터(각각의 상황마다 써야하는 함수가 다르기 때문)
		bool (*initializer)(struct page *, enum vm_type, void *);
		// 타입에 맞는 초기화 함수 지정
		switch (VM_TYPE(type)){
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
			default:
				NOT_REACHED();
				break;
		}
		//VM_UNINIT 타입으로 페이지 생성
		uninit_new(p, upage, init, type, aux, initializer); 
		// uninit_new를 호출한 후에는 필드를 수정해야 한다.
		p->writable = writable;
		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, p);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
// 추가 페이지 테이블에서 va에 해당하는 페이지를 찾는 함수
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL; // 페이지 포인터를 생성
	/* TODO: Fill this function. */
	page = (struct page *)malloc(sizeof(struct page)); // 해당 페이지에 메모리 할당
	page->va = pg_round_down(va); // 페이지의 가상주소를 페이지의 끝부분을 가르키도록 함
	struct hash_elem *e = hash_find(&spt->hash_table, &page->hash_elem); // 해시테이블에서 요소를 찾아옴
	free(page); // 사용을 완료한 페이지 메모리 해제
	if (e == NULL){ // 요소가 없을 경우
		return NULL;
	}
	return hash_entry(e, struct page, hash_elem); // e라는 요소를 가지고 있는 페이지를 반환
}

/* Insert PAGE into spt with validation. */
// 보조 페이지 테이블(supplementary page table)에 페이지를 삽입하는 함수, 성공 : true, 실패 : false
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	/* 
	 *	`hash_insert()` 함수 내부에서 가상 주소가 이미 supplementary page table에 존재하는지 확인한다.
	 *	`struct hash_elem *old = find_elem (h, bucket, new);`
	 *	
	*/
	struct hash_elem *e = hash_insert(&spt->hash_table, &page->hash_elem);
	if (e == NULL) {
		succ = true;
	}
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	if(page != NULL){
		vm_dealloc_page (page);
	}
	return;
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
static struct frame *vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	// 사용자 풀에서 페이지 할당 받기 - 할당 받은 물리 메모리 주소 반환
	void *addr = palloc_get_page(PAL_USER);
	if(addr == NULL){
		PANIC("todo");
	}

	frame = (struct frame *)malloc(sizeof(struct frame));
	frame->kva = addr;
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	// todo: 스택 크기를 증가시키기 위해 anon page를 하나 이상 할당하여 주어진 주소(addr)가 더 이상 예외 주소(faulted address)가 되지 않도록 합니다.
	// todo: 할당할 때 addr을 PGSIZE로 내림하여 처리
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
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

	if (addr == NULL)
        return false;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	void *vaddr = pg_round_down(addr);

	if (is_kernel_vaddr(vaddr))
        return false;

	// 접근한 메모리의 physical page가 존재하지 않은 경우
	if (!not_present)
		return false;

	// todo: 페이지 폴트가 스택 확장에 대한 유효한 경우인지를 확인해야 합니다.
	void *rsp = f->rsp; // user access인 경우 rsp는 유저 stack을 가리킨다.
	if (!user)			// kernel access인 경우 thread에서 rsp를 가져와야 한다.
		rsp = thread_current()->rsp;

	// 스택 확장으로 처리할 수 있는 폴트인 경우, vm_stack_growth를 호출
	if (USER_STACK - (1 << 20) <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK)
		vm_stack_growth(addr);
	else if (USER_STACK - (1 << 20) <= rsp && rsp <= addr && addr <= USER_STACK)
		vm_stack_growth(addr);

	page = spt_find_page(spt, vaddr);
	if(page == NULL)
		return false;

	// write 불가능한 페이지에 write 요청한 경우
	if (write == 1 && page->writable == 0) 
		return false;

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
// va로 page를 찾아서 vm_do_claim_page를 호출하는 함수
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
    // spt에서 va에 해당하는 page 찾기
	page = spt_find_page(&thread_current()->spt, va);
	if(page == NULL)
		return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
// 페이지와 프레임을 연결하는 함수
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	bool result = pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable);
	if(result){
		return swap_in (page, frame->kva);
	}
	return false;
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash_table, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
			// TODO: 보조 페이지 테이블을 src에서 dst로 복사합니다.
			// TODO: src의 각 페이지를 순회하고 dst에 해당 entry의 사본을 만듭니다.
			// TODO: uninit page를 할당하고 그것을 즉시 claim해야 합니다.
			struct hash_iterator i;
			hash_first(&i, &src->hash_table);
			while(hash_next(&i)){
        		// src_page 정보
				struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
				enum vm_type type = src_page->operations->type;
				void *va = src_page->va;
				bool writable = src_page->writable;

				/* 1) type이 uninit이면 */
				if(type == VM_UNINIT){
					// uninit page 생성 & 초기화
					vm_alloc_page_with_initializer(VM_ANON, va, writable, src_page->uninit.init, src_page->uninit.aux);
					continue;
				}
				/* 2) type이 file이면? */
				else if(type == VM_FILE){
					struct vm_entry *vme = (struct vm_entry *)malloc(sizeof(struct vm_entry));
					vme->f = src_page->file.file;
					vme->offset = src_page->file.offset;
					vme->read_bytes = src_page->file.read_bytes;
					vme->zero_bytes = src_page->file.zero_bytes;

					if(!vm_alloc_page_with_initializer(type, va, writable, NULL, vme)){
						return false;
					}
					struct page *page = spt_find_page(dst, va);
					file_backed_initializer(page, type, NULL);
					page->frame = src_page->frame;
					pml4_set_page(thread_current()->pml4, page->va, src_page->frame->kva, src_page->writable);
				}
				else{
					/* 2) type이 uninit이 아니면 */
					// uninit page 생성 & 초기화
					if(!vm_alloc_page(type, va, writable)){
						// init이랑 aux는 Lazy Loading에 필요함
            			// 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요 없음
						return false;
					}
					// vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
					if(!vm_claim_page(va)){
						return false;
					}
					// 매핑된 프레임에 내용 로딩
					struct page *dst_page = spt_find_page(dst, va);
					memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
				}
			}
			return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// todo: 페이지 항목들을 순회하며 테이블 내의 페이지들에 대해 destroy(page)를 호출
	hash_clear(&spt->hash_table, hash_page_destroy);
	
	/** hash_destroy가 아닌 hash_clear를 사용해야 하는 이유
	 * 여기서 hash_destroy 함수를 사용하면 hash가 사용하던 메모리(hash->bucket) 자체도 반환한다.
	 * process가 실행될 때 hash table을 생성한 이후에 process_clean()이 호출되는데,
	 * 이때는 hash table은 남겨두고 안의 요소들만 제거되어야 한다.
	 * 따라서, hash의 요소들만 제거하는 hash_clear를 사용해야 한다.
	 */
}

// 들어온 요소 p의 가상 주소 값을 unsigned int형(양의 정수값) 범위의 값으로 변경
unsigned 
page_hash(const struct hash_elem *p_, void *aux UNUSED){
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes (&p->va, sizeof p->va);
}

// 가상 주소의 값을 정렬해서 반환
bool 
page_less(const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED){
	const struct page *a = hash_entry (a_, struct page, hash_elem);
	const struct page *b = hash_entry (b_, struct page, hash_elem);

	return a->va < b->va;
}

void hash_page_destroy(struct hash_elem *e, void *aux){
	struct page *p = hash_entry(e, struct page, hash_elem);
	vm_dealloc_page(p);
}