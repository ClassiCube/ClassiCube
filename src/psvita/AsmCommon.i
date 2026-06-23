// ===============================
// FUNCTION MACROS
// ===============================
#define BEG_FUNC(name_) \
	.global name_; \
	.align 4; \
	name_:

#define END_FUNC(name_) \
	.size name_, . - name_; \
	.type name_, %function;
