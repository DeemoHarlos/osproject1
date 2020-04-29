#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/uaccess.h>

// system call number 437
// reference: https://medium.com/@lee1003094395/adding-a-system-call-which-can-pass-a-userspace-string-b245105bed38
// reference: https://stackoverflow.com/questions/27365372/system-calls-with-struct-parameters-linux

SYSCALL_DEFINE1(get_time_ns, struct timespec *, t) {
	struct timespec now;
	getnstimeofday(&now);
	if (copy_to_user(t, &now, sizeof(struct timespec))) return -1;
	return 0;
}