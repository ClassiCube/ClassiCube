
#define BEG_THUMB_FUNC(name_) \
	.global name_; \
	.thumb_func; \
	.align 2; \
	name_:

#define BEG_ARM_FUNC(name_) \
	.global name_; \
	.arm; \
	.align 4; \
	name_:

#define END_FUNC(name_) \
	.size name_, . - name_; \
	.type name_, %function;


// ===============================
// FUNCTIONS IN TEXT SECTION
// ===============================
.section text


// ===============================
// FUNCTIONS IN IWRAM SECTION
// ===============================
.section .iwram,"ax",%progbits

#define R_CUR r0 // r0 = beg address (incremented in function)
#define R_END r1 // r1 = end address
#define R_VAL r2 // r2 = value to fill

BEG_ARM_FUNC(fastset_256_bytes)
	// Spill callee saved registers
	stmfd sp!, {r4-r9}

	// Clone 'value' for 'store multi CPU registers' loop
	mov	r3, R_VAL
	mov	r4, R_VAL
	mov r5, R_VAL
	mov r6, R_VAL
	mov r7, R_VAL
	mov r8, R_VAL
	mov r9, R_VAL

fastset_loop:
	cmp R_CUR, R_END
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	stmltia R_CUR!, {r2-r9}
	blt fastset_loop

	// Restore saved CPU registers
	ldmfd  sp!, {r4-r9}
	bx lr
END_FUNC(fastset_256_bytes)


// ===============================
// FUNCTIONS IN EWRAM SECTION
// ===============================
.section .ewram,"ax",%progbits
BEG_THUMB_FUNC(nocash_log)
	// nocash looks for this specific pattern
	mov 	r12, r12
	b		nocash_log_return
	.short	0x6464, 0x0000
.global nocash_msg // Platform_GBA writes directly to this
nocash_msg:
	.space	82

nocash_log_return:
	bx		lr
END_FUNC(nocash_log)
