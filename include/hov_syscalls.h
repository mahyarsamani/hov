/*
 * hov_syscalls.h - LD_PRELOAD interposer for HOV memory
 *
 * This library is built as a standalone shared object (libhov_syscalls.so)
 * and loaded via LD_PRELOAD to transparently bounce read()/write() calls
 * that target VM_PFNMAP memory mapped from /dev/hov.
 *
 * It is fully self-contained: it discovers the HOV memory range by parsing
 * /proc/self/maps at runtime, requiring no symbols from the main binary.
 */

#ifndef HOV_SYSCALLS_H
#define HOV_SYSCALLS_H

#include <sys/types.h>

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#endif /* HOV_SYSCALLS_H */
