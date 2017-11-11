#; -*- fundamental -*-



#define save_registers \
	pushq %rbx;    \
	pushq %rcx;    \
	pushq %rdx;    \
	pushq %rdi;    \
	pushq %rsi;    \
	pushq %r8;     \
	pushq %r9;     \
	pushq %r10;    \
	pushq %r11;    \
	pushq %r12;    \
	pushq %r13;    \
	pushq %r14;    \
	pushq %r15;    \
	pushq %rbp;


#define restore_registers \
	popq %rbp;	  \
	popq %r15;	  \
	popq %r14;	  \
	popq %r13;	  \
	popq %r12;	  \
	popq %r11;	  \
	popq %r10;	  \
	popq %r9;	  \
	popq %r8;	  \
	popq %rsi;	  \
	popq %rdi;	  \
	popq %rdx;	  \
	popq %rcx;	  \
	popq %rbx;	  \




.align 8
.globl __switch_to_stack
__switch_to_stack:
	save_registers;
	movq %rsp,   (%rsi);
	movq (%rdi), %rsp;
	
	movq %rdx,   %rdi;
	movq %rcx,   %rsi;
	call pet_thread_cleanup;

	restore_registers;
	ret;



.align 8
.globl __abort_to_stack
__abort_to_stack:
	movq (%rdi), %rsp;
	restore_registers;
	ret;