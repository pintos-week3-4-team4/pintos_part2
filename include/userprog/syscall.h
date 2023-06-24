#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"

struct lock filesys_lock;

void syscall_init (void);
void check_address (void *addr);
void *mmap(void *addr, size_t length, int writable, int fd, off_t offset);
void munmap(void *addr);
void halt (void);
void exit (int status);
tid_t fork(const char *thread_name, struct intr_frame *f);
int exec (const char *file_name);
int wait (tid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
