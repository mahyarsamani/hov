/*
 * hov_ioctl.h — Shared ioctl definitions for /dev/hov
 *
 * Included by both the kernel driver (hov_drv.c) and
 * userspace library (hov_pool.c).
 */
#ifndef HOV_IOCTL_H
#define HOV_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define HOV_IOC_MAGIC  'H'

/* Query the total pool size in bytes.
 * arg: pointer to unsigned long long (output) */
#define HOV_GET_MEM_SIZE   _IOR(HOV_IOC_MAGIC, 0, unsigned long long)

/* Query the physical base address.
 * arg: pointer to unsigned long long (output) */
#define HOV_GET_PHYS_BASE  _IOR(HOV_IOC_MAGIC, 1, unsigned long long)

#endif /* HOV_IOCTL_H */
