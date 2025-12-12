.global nocash_log
.global nocash_msg

// BEG nocash_log
.section .ewram,"ax",%progbits
.thumb_func
.align 2

// nocash looks for this specific pattern
nocash_log:
	mov 	r12, r12
	b		nocash_log_return
	.short	0x6464, 0x0000
nocash_msg:
	.space	82

nocash_log_return:
	bx		lr

.size nocash_log, .-nocash_log
.type nocash_log, %function
// END nocash_log
