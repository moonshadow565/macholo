// This is borrowed from darling, it might be incorrect...
// See: https://github.com/darlinghq/darling-libobjc2/blob/master/darling/x86-64/objc_msgSendSuper.S

.text
.globl objc_msgSend_noarg
.globl objc_msgSendSuper
.globl objc_msgSendSuper2 // requires extracting superclass from Class
.globl objc_msgSendSuper2_stret
.globl objc_msgSend_fixup
.globl objc_msgSendSuper2_fixup
.globl objc_msgSendSuper2_stret_fixup
.globl objc_msgSend_fpret_fixup
.globl objc_msgSend_fp2ret_fixup
.globl objc_msgSend_stret_fixup

#define SaveRegisters \
	pushq %rdi ;\
	pushq %rsi ;\
	pushq %rdx ;\
	pushq %rcx ;\
	pushq %r8 ;\
	pushq %r9

#define  RestoreRegisters \
	popq %r9 ;\
	popq %r8 ;\
	popq %rcx ;\
	popq %rdx ;\
	popq %rsi ;\
	popq %rdi

objc_msgSend_noarg:
	jmp objc_msgSend@PLT

objc_msgSendSuper2:
	SaveRegisters
	
	// Make a copy of the struct on stack
	movq (%rdi), %rax
	movq %rax, -16(%rsp)
	movq 8(%rdi), %rax
	movq 8(%rax), %rax // load superclass
	movq %rax, -8(%rsp)

	subq $16, %rsp
	movq %rsp, %rdi
	
	call objc_msg_lookup_super@PLT
	
	addq $16, %rsp
	
	RestoreRegisters
	
	movq (%rdi), %rdi
	jmpq *%rax
	
objc_msgSendSuper:
	SaveRegisters
	
	call objc_msg_lookup_super@PLT
	
	RestoreRegisters
	
	movq (%rdi), %rdi
	jmpq *%rax

objc_msgSendSuper2_stret:
	SaveRegisters
	
	// Make a copy of the struct on stack
	movq (%rsi), %rax
	movq %rax, -16(%rsp)
	movq 8(%rsi), %rax
	movq 8(%rax), %rax // load superclass
	movq %rax, -8(%rsp)

	subq $16, %rsp
	movq %rsp, %rdi
	movq %rdx, %rsi
	
	call objc_msg_lookup_super@PLT
	
	addq $16, %rsp
	
	RestoreRegisters
	
	movq (%rsi), %rsi
	jmpq *%rax

objc_msgSendSuper_stret:
	SaveRegisters
	
	movq %rsi, %rdi
	movq %rdx, %rsi
	
	call objc_msg_lookup_super@PLT
	
	RestoreRegisters
	
	movq (%rsi), %rsi
	jmpq *%rax

objc_msgSend_fp2ret_fixup:
objc_msgSend_fpret_fixup:
	movq 8(%rsi), %rsi
	jmp objc_msgSend_fpret@PLT

objc_msgSend_fixup:
	movq 8(%rsi), %rsi
	jmp objc_msgSend@PLT

objc_msgSendSuper2_fixup:
	movq 8(%rsi), %rsi
	jmp objc_msgSendSuper2@PLT

objc_msgSend_stret_fixup:
	movq 8(%rdx), %rdx
	jmp objc_msgSend_stret@PLT
	
objc_msgSendSuper2_stret_fixup:
	movq 8(%rdx), %rdx
	jmp objc_msgSendSuper2_stret@PLT

.section ".note.GNU-stack","",@progbits
