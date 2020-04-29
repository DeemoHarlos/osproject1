#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

// system call number 436
// reference: https://medium.com/@lee1003094395/adding-a-system-call-which-can-pass-a-userspace-string-b245105bed38

SYSCALL_DEFINE2(print_kernel, const char *, user_str, unsigned, len) {
	char buf[256];
	if (len > 256) len = 256;
	if (copy_from_user(buf, user_str, len)) return -1;
	printk("%s\n", buf);
	return 0;
}