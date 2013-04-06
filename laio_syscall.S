/* $Id: laio_syscall.S,v 1.4 2004/01/13 00:18:26 anupamc Exp $ */
/*
.LC0:
	.string	"env = %p\n"
*/
	.text
	.p2align 2,,3
.globl laio_syscall
	.type	laio_syscall, @function
laio_syscall:
/*
	pushl	$sched_data+96
	pushl	$.LC0
	call	printf
	addl	$8, %esp
*/
	pushl	$sched_data+96
	call	_setjmp
	addl	$4, %esp
	testl	%eax, %eax
	jne	.L2
	movl	current_thread, %eax
	movl	%eax, sched_data+4
	movl	(%esp), %eax
	movl	%eax, foo
	movl	$.L1, (%esp)
	jmp	syscall
.L1:
	movl	$0, sched_data+4
.L3:
	jmp	*foo
.L2:
	call	__error
	movl	$36, (%eax)
	movl	$-1, %eax
	jmp	.L3
	.size	laio_syscall, .-laio_syscall
