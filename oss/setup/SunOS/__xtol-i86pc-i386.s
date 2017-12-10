.file	"__xtol-i86pc-i386.s"
.text
.globl __xtol
	.type	__xtol, @function
__xtol:
	subl    $0x8,%esp
	fnstcw  0x2(%esp)
	movw    0x2(%esp),%ax
	movw    %ax,%cx
	andw    $0xc00,%cx
	orw     $0xc00,%ax
	movw    %ax,0x0(%esp)
	fldcw  0x0(%esp)
	fistpl 0x4(%esp)
	fstcw  0x0(%esp)
	movw    0x0(%esp),%ax
	andw    $0xf3ff,%ax
	orw     %cx,%ax
	movw    %ax,0x0(%esp)
	fldcw  0x0(%esp)
	movl    0x4(%esp),%eax
	addl    $0x8,%esp
	ret
	.size	__xtol, .-__xtol
