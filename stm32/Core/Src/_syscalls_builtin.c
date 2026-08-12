/*
 * syscalls.c - Minimal system call stubs for newlib
 * Needed for printf/snprintf/puts to work on bare-metal
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

extern UART_HandleTypeDef huart2;

int _write(int fd, const char *ptr, int len) {
    (void)fd;
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
    return len;
}

int _read(int fd, char *ptr, int len) {
    (void)fd;
    (void)ptr;
    return 0;  /* No input for now */
}

void _exit(int status) {
    (void)status;
    while(1);  /* Trap */
}

int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; (void)st; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd; (void)ptr; (void)dir; return 0; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
