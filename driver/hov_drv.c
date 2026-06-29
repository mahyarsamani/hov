#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>

#define DEV_NAME "hov"
#define DEV_MODE ((umode_t)0666) /* All users have RW access to this device */

unsigned long long hov_phys_base;
module_param(hov_phys_base, ullong, 0644);
MODULE_PARM_DESC(hov_phys_base, "Physical base address for HOV memory range");

unsigned long long hov_mem_size;
module_param(hov_mem_size, ullong, 0644);
MODULE_PARM_DESC(hov_mem_size, "Size of HOV memory range");

static int hov_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long long pfn;

    if (hov_phys_base == 0 || hov_mem_size == 0) {
        pr_err("hov_drv: missing or invalid physical base address or size\n");
        return -EINVAL;
    }

    /* Only map from offset 0, matching the mapped region size exactly. */
    if (vma->vm_pgoff != 0) {
        pr_err("hov_drv: mmap must have offset 0 (requested offset: %lu)\n", vma->vm_pgoff);
        return -EINVAL;
    }

    if (size > hov_mem_size) {
        pr_err("hov_drv: requested mmap size (%lu) exceeds available size (%llu)\n", size, hov_mem_size);
        return -EINVAL;
    }

    /* Calculate Page Frame Number from Physical Address */
    pfn = hov_phys_base >> PAGE_SHIFT;

    /* Prevent memory caching for unmanaged physical memory.
     * Often pgprot_noncached() is used for strictly MMIO registers,
     * but since this is actual RAM, we might want it cached if possible. 
     * We'll stick with default VMA protections unless this causes issues.
     */
    /* vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); */

    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
        pr_err("hov_drv: remap_pfn_range failed\n");
        return -EAGAIN;
    }

    pr_info("hov_drv: mmap successful. vma start: %lx, size: %lu, pfn: %llu\n",
            vma->vm_start, size, pfn);

    return 0;
}

static const struct file_operations hov_fops = {
    .owner = THIS_MODULE,
    .mmap  = hov_mmap,
};

static struct miscdevice hov_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEV_NAME,
    .fops  = &hov_fops,
    .mode  = DEV_MODE,
};

static int __init hov_init(void)
{
    int ret;

    ret = misc_register(&hov_miscdev);
    if (ret) {
        pr_err("hov_drv: failed to register misc device\n");
        return ret;
    }

    pr_info("hov_drv: initialized with phys_base: %llx, mem_size: %llu bytes\n",
            hov_phys_base, hov_mem_size);

    return 0;
}

static void __exit hov_exit(void)
{
    misc_deregister(&hov_miscdev);
    pr_info("hov_drv: exiting\n");
}

module_init(hov_init);
module_exit(hov_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Antigravity");
MODULE_DESCRIPTION("HOV Memory Allocator Bridge Driver");
