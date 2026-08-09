/**
 * @file    syscalls.c
 * @brief   newlib syscall retarget
 *
 * newlib-nano needs _sbrk for malloc/printf internals.
 * The heap is backed by a static array (large enough for printf's internal use).
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#undef errno
extern int errno;

/* Simple static heap (used by printf internals; does not need to be large) */
#define HEAP_SIZE    2048
static unsigned char _heap[HEAP_SIZE];
static size_t _heap_offset = 0;

/* _sbrk - heap allocator for newlib */
void *_sbrk(int incr)
{
    if (_heap_offset + (size_t)incr > HEAP_SIZE) {
        errno = ENOMEM;
        return (void *)-1;
    }
    void *ptr = &_heap[_heap_offset];
    _heap_offset += incr;
    return ptr;
}

/* _write is implemented in main.c */

int _close(int file)        { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file)        { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
void _exit(int status)      { (void)status; while (1) {} }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
pid_t _getpid(void)         { return 0; }
