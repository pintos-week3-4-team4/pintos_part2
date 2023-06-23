#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "vm/vm.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
	lock_init (&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// printf ("system call!\n");
	// thread_exit ();
	#ifdef VM
		thread_current()->rsp = f->rsp;
	#endif
	switch (f->R.rax) {
		case SYS_HALT:
			halt ();
			break;
		case SYS_EXIT:
			exit (f->R.rdi);
			break;
		case SYS_FORK:
			// memcpy (&thread_current ()->ptf, f, sizeof (struct intr_frame));
			f->R.rax = fork (f->R.rdi, f);
			break;
		case SYS_EXEC:
			f->R.rax = exec (f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = wait (f->R.rdi);
			break; 
		case SYS_CREATE:
			f->R.rax = create (f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove (f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open (f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize (f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read (f->R.rdi, f->R.rsi, f->R.rdx);
			break;  
		case SYS_WRITE:      
			f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek (f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell (f->R.rdi);
			break;
		case SYS_CLOSE:
			close (f->R.rdi);
			break;
		default:
			exit (-1);
			break;
	}
}

void check_address (void *addr) {	
	if (addr == NULL || !is_user_vaddr(addr))
		exit (-1);
}

void halt (void) {
	power_off ();
}

void exit (int status) {
	struct thread *cur = thread_current ();

	cur->exit_status = status;
	printf ("%s: exit(%d)\n", cur->name, status);
	thread_exit ();
}

tid_t fork(const char *thread_name, struct intr_frame *f) {
	check_address (thread_name);
	return process_fork(thread_name, f);
	// return process_fork (thread_name, &thread_current ()->ptf);
}

int exec (const char *file_name) {
	check_address (file_name);

	// process.c 파일의 process_create_initd 함수와 유사하다.
	// 단, 스레드를 새로 생성하는 건 fork에서 수행하므로
	// 이 함수에서는 새 스레드를 생성하지 않고 process_exec을 호출한다.

	// process_exec 함수 안에서 filename을 변경해야 하므로
	// 커널 메모리 공간에 cmd_line의 복사본을 만든다.
	// (현재는 const char* 형식이기 때문에 수정할 수 없다.)

	char *file_name_copy;
	file_name_copy = palloc_get_page(0);
	if (file_name_copy == NULL)
		exit(-1);							  // 메모리 할당 실패 시 status -1로 종료한다.
	strlcpy(file_name_copy, file_name, PGSIZE); // cmd_line을 복사한다.

	if (process_exec(file_name_copy) == -1)
		exit(-1); // 실패 시 status -1로 종료한다.

	// int file_size = strlen (file_name) + 1;
	// char *fn_copy = palloc_get_page (PAL_ZERO);

	// if (!fn_copy) {
	// 	exit (-1);
	// 	return -1;
	// }

	// strlcpy (fn_copy, file_name, file_size);

	// if (process_exec (fn_copy) == -1) {
	// 	exit (-1);
	// 	return -1;
	// }
}

int wait (tid_t pid) {
  	return process_wait (pid);
}

bool create (const char *file, unsigned initial_size) {
	lock_acquire(&filesys_lock);
	check_address(file);
	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return success;
}

bool remove (const char *file) {
	check_address (file);
	return filesys_remove (file);
}

int open (const char *file_name) {
	check_address (file_name);
	
	lock_acquire(&filesys_lock);
	struct thread *cur = thread_current ();
	struct file *file = filesys_open(file_name);

	if (file == NULL) {
		lock_release(&filesys_lock);
		return -1;
	}

	int fd = process_add_file(file);

	if (fd == -1) {
		file_close(file);
	}

	lock_release(&filesys_lock);
	return fd;
}

int filesize (int fd) {
	struct file *file = process_get_file(fd);
	if (file == NULL)
		return -1;
	return file_length (file);
}

int read (int fd, void *buffer, unsigned size) {
	check_address (buffer);

	if (fd == STDOUT_FILENO)
		return -1;

	char *ptr = (char *)buffer;
	int read_byte = 0;

	lock_acquire (&filesys_lock);

	if (fd == STDIN_FILENO) {
		for (int i = 0; i < size; i++)
		{
			*ptr++ = input_getc();
			read_byte++;
		}
		lock_release(&filesys_lock);
		return read_byte;
	}

	if (fd < 2) {
		lock_release(&filesys_lock);
		return -1;
	}

	struct file *file = process_get_file(fd);
	if (file == NULL) {
		lock_release(&filesys_lock);
		return -1;
	}

	struct page *page = spt_find_page(&thread_current()->spt, buffer);
	if (page && !page->writable)
	{
		lock_release(&filesys_lock);
		exit(-1);
	}

	if (file) {	
		read_byte = file_read (file, buffer, size);
		lock_release (&filesys_lock);
		return read_byte;
	}

	lock_release (&filesys_lock);
	return -1;
}

int write (int fd UNUSED, const void *buffer, unsigned size) {
	check_address (buffer);

	if (fd == 0)
		return -1;

	if (fd == 1) {
		lock_acquire (&filesys_lock);
		putbuf (buffer, size);
		lock_release (&filesys_lock);
		return size;
	}

	struct file *file = process_get_file(fd);

	if (file) {
		lock_acquire (&filesys_lock);
		int write_byte = file_write (file, buffer, size);
		lock_release (&filesys_lock);
		return write_byte;
	}
}

void seek (int fd, unsigned position) {
	struct file *curfile = process_get_file(fd);
	if (curfile == NULL)
		return;
	file_seek (curfile, position);
}

unsigned tell (int fd) {
	struct file *curfile = process_get_file(fd);
	if (curfile == NULL)
		return;
	return file_tell (curfile);
}

void close (int fd) {
	struct file * file = process_get_file(fd);
	if (file == NULL)
		return;
	lock_acquire (&filesys_lock);
	thread_current ()->fdt[fd] = NULL;
	file_close (file);
	process_close_file(fd);
	lock_release (&filesys_lock);
}
