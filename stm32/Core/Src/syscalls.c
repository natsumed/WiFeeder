#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

int _write(int fd, const char *ptr, int len) {
    (void)fd;
    volatile uint32_t *isr = (volatile uint32_t*)0x4000441C;
    volatile uint32_t *tdr = (volatile uint32_t*)0x40004428;
    for (int i = 0; i < len; i++) {
        while (!(*isr & (1UL << 7)));
        *tdr = (uint8_t)ptr[i];
    }
    return len;
}

int _read(int fd, char *ptr, int len) { (void)fd;(void)ptr;(void)len; return 0; }
void _exit(int status) { (void)status; while(1); }
int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd;(void)st; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd;(void)ptr;(void)dir; return 0; }
void *_sbrk(int incr) { extern char _end; static char *h = 0; if (!h) h = &_end; char *p = h; h += incr; return p; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid;(void)sig; errno = EINVAL; return -1; }
void _init(void) {}
