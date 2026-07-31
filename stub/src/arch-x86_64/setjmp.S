// Darwin x86_64 jmp_buf is int[37] == 148 bytes; glibc's __jmp_buf_tag is 200.
// Forwarding setjmp/longjmp lets glibc write 52 bytes past the guest's buffer
// (docs/surface-audit/VERDICTS.md). These four own the buffer instead, with a
// private layout -- guests only ever pass a jmp_buf between this pair, so the
// layout is ours to choose as long as it fits in 148 bytes:
//   0..55   rbx rbp r12 r13 r14 r15 (callee-saved)
//   48      rsp after return        56  return rip
//   64      mxcsr (4)               68  x87 control word (2)
//   72      signal mask (8 -- raw kernel mask via rt_sigprocmask(..., 8),
//           NOT glibc sigprocmask whose sigset_t is 128 bytes and would
//           overflow this buffer)
//   88      "mask saved" flag: setjmp sets 1, _setjmp sets 0
// Darwin semantics: setjmp/longjmp save/restore the mask, _setjmp/_longjmp don't.

#define OFF_RBX   0
#define OFF_RBP   8
#define OFF_R12   16
#define OFF_R13   24
#define OFF_R14   32
#define OFF_R15   40
#define OFF_RSP   48
#define OFF_RIP   56
#define OFF_MXCSR 64
#define OFF_FCW   68
#define OFF_MASK  72
#define OFF_HAVEMASK 88

#define SYS_rt_sigprocmask 14
#define SIG_BLOCK   0
#define SIG_SETMASK 2

.macro SAVE_CORE
	movq %rbx, OFF_RBX(%rdi)
	movq %rbp, OFF_RBP(%rdi)
	movq %r12, OFF_R12(%rdi)
	movq %r13, OFF_R13(%rdi)
	movq %r14, OFF_R14(%rdi)
	movq %r15, OFF_R15(%rdi)
	leaq 8(%rsp), %rax
	movq %rax, OFF_RSP(%rdi)
	movq (%rsp), %rax
	movq %rax, OFF_RIP(%rdi)
	stmxcsr OFF_MXCSR(%rdi)
	fnstcw OFF_FCW(%rdi)
.endm

.macro RESTORE_AND_JUMP
	movq OFF_RBX(%rdi), %rbx
	movq OFF_RBP(%rdi), %rbp
	movq OFF_R12(%rdi), %r12
	movq OFF_R13(%rdi), %r13
	movq OFF_R14(%rdi), %r14
	movq OFF_R15(%rdi), %r15
	ldmxcsr OFF_MXCSR(%rdi)
	fldcw OFF_FCW(%rdi)
	// longjmp(env, 0) must deliver 1 at the setjmp return
	movl %esi, %eax
	testl %eax, %eax
	jnz 1f
	movl $1, %eax
1:
	movq OFF_RSP(%rdi), %rsp
	jmpq *OFF_RIP(%rdi)
.endm

.text

.globl "$darwin$setjmp"
"$darwin$setjmp":
	SAVE_CORE
	// current mask -> OFF_MASK: rt_sigprocmask(SIG_BLOCK, NULL, &mask, 8)
	// syscall clobbers only rax/rcx/r11, so no register juggling needed
	leaq OFF_MASK(%rdi), %rdx
	movq %rdi, %r8              // syscall doesn't touch r8; keep env across it
	xorl %edi, %edi             // SIG_BLOCK
	xorl %esi, %esi             // set = NULL
	movl $8, %r10d              // sigsetsize
	movl $SYS_rt_sigprocmask, %eax
	syscall
	movq %r8, %rdi
	movl $1, OFF_HAVEMASK(%rdi)
	xorl %eax, %eax
	ret

.globl "$darwin$_setjmp"
"$darwin$_setjmp":
	SAVE_CORE
	movl $0, OFF_HAVEMASK(%rdi)
	xorl %eax, %eax
	ret

.globl "$darwin$longjmp"
"$darwin$longjmp":
	cmpl $0, OFF_HAVEMASK(%rdi)
	je 2f
	// rt_sigprocmask(SIG_SETMASK, &mask, NULL, 8)
	movq %rdi, %r8
	movq %rsi, %r9              // value survives in r9 (syscall preserves it)
	leaq OFF_MASK(%rdi), %rsi
	movl $SIG_SETMASK, %edi
	xorl %edx, %edx
	movl $8, %r10d
	movl $SYS_rt_sigprocmask, %eax
	syscall
	movq %r8, %rdi
	movq %r9, %rsi
2:
	RESTORE_AND_JUMP

.globl "$darwin$_longjmp"
"$darwin$_longjmp":
	RESTORE_AND_JUMP
