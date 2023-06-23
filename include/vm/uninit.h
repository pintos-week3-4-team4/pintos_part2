#ifndef VM_UNINIT_H
#define VM_UNINIT_H
#include "vm/vm.h"

struct page;
enum vm_type;

typedef bool vm_initializer (struct page *, void *aux);

/* Uninitlialized page. The type for implementing the
 * "Lazy loading". */
/* 초기화되지 않은 페이지. "Lazy loading"을 구현하기 위한 타입입니다. */
struct uninit_page {
	/* Initiate the contets of the page */
	/* 페이지 내용을 초기화 합니다 */
	vm_initializer *init;
	enum vm_type type;
	void *aux;
	/* Initiate the struct page and maps the pa to the va */
	/* struct page를 초기화하고 물리 주소(pa)를 가상 주소(va)에 매핑합니다. */
	bool (*page_initializer) (struct page *, enum vm_type, void *kva);
};

void uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *kva));
#endif
