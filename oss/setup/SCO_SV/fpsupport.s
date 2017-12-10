	.file	"fpsupport_i86pc.c"
	.text
.globl oss_fp_check
	.type	oss_fp_check, @function
oss_fp_check:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
/APP
	pushfl ; popl %edx
/NO_APP
	andl	$-2097153, %edx
/APP
	pushl %edx ; popfl
	pushfl ; popl %eax
/NO_APP
	movl	%eax, %ecx
	orl	$-1, %edx
	andl	$2097152, %ecx
	jne	.L1
	orl	$2097152, %eax
/APP
	pushl %eax ; popfl
	pushfl ; popl %eax
/NO_APP
	movl	$-2, %edx
	testl	$2097152, %eax
	je	.L1
	movl	$1, %eax
/APP
	cpuid
/NO_APP
	andl	$16777216, %edx
	cmpl	$1, %edx
	sbbl	%ecx, %ecx
	andl	$-4, %ecx
	leal	1(%ecx), %edx
.L1:
	movl	%edx, %eax
	popl	%ebx
	popl	%ebp
	ret
	.size	oss_fp_check, .-oss_fp_check
.globl oss_fp_save
	.type	oss_fp_save, @function
oss_fp_save:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%eax
	pushl	%eax
	movl	12(%ebp), %ecx
/APP
	movl %cr0,%eax
/NO_APP
	movl	%eax, (%ecx)
/APP
	movl %cr4,%edx
/NO_APP
	movl	%edx, 4(%ecx)
	andl	$-15, %eax
/APP
	movl %eax,%cr0
/NO_APP
	orb	$6, %dh
/APP
	movl %edx,%cr4
/NO_APP
	movl	8(%ebp), %edx
/APP
	fxsave (%edx)
	fninit
	fwait
/NO_APP
	movl	$8064, -8(%ebp)
	movl	$0, -4(%ebp)
/APP
	ldmxcsr -8(%ebp)
	movl %cr0,%edx
/NO_APP
	movl	%edx, 8(%ecx)
	leave
	ret
	.size	oss_fp_save, .-oss_fp_save
.globl oss_fp_restore
	.type	oss_fp_restore, @function
oss_fp_restore:
	pushl	%ebp
	movl	%esp, %ebp
	movl	12(%ebp), %edx
/APP
	fwait
/NO_APP
	movl	8(%ebp), %ecx
/APP
	fxrstor (%ecx)
/NO_APP
	movl	(%edx), %ecx
/APP
	movl %ecx,%cr0
/NO_APP
	movl	4(%edx), %ecx
/APP
	movl %ecx,%cr4
/NO_APP
	popl	%ebp
	ret
	.size	oss_fp_restore, .-oss_fp_restore
