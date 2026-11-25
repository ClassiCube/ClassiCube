/* Field names differ depending on __DARWIN_UNIX03 */
#include <signal.h>
#include <sys/ucontext.h>

#if __DARWIN_UNIX03
/* See /usr/include/mach/i386/_structs.h (macOS 10.5+) */
void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->__ss.__eax, &r->__ss.__ebx, &r->__ss.__ecx, &r->__ss.__edx);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->__ss.__esi, &r->__ss.__edi, &r->__ss.__ebp, &r->__ss.__esp);
	String_Format1(str, "eip=%x"                      _NL, &r->__ss.__eip);
#elif defined __x86_64__
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->__ss.__rax, &r->__ss.__rbx, &r->__ss.__rcx, &r->__ss.__rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->__ss.__rsi, &r->__ss.__rdi, &r->__ss.__rbp, &r->__ss.__rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__ss.__r8,  &r->__ss.__r9,  &r->__ss.__r10, &r->__ss.__r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__ss.__r12, &r->__ss.__r13, &r->__ss.__r14, &r->__ss.__r15);
	String_Format1(str, "rip=%x"                      _NL, &r->__ss.__rip);
#elif defined __aarch64__
	/* /usr/include/mach/arm/_structs.h */
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__ss.__x[0],  &r->__ss.__x[1],  &r->__ss.__x[2],  &r->__ss.__x[3]);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__ss.__x[4],  &r->__ss.__x[5],  &r->__ss.__x[6],  &r->__ss.__x[7]);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__ss.__x[8],  &r->__ss.__x[9],  &r->__ss.__x[10], &r->__ss.__x[11]);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__ss.__x[12], &r->__ss.__x[13], &r->__ss.__x[14], &r->__ss.__x[15]);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__ss.__x[16], &r->__ss.__x[17], &r->__ss.__x[18], &r->__ss.__x[19]);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__ss.__x[20], &r->__ss.__x[21], &r->__ss.__x[22], &r->__ss.__x[23]);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__ss.__x[24], &r->__ss.__x[25], &r->__ss.__x[26], &r->__ss.__x[27]);
	String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, &r->__ss.__x[28], &r->__ss.__fp,    &r->__ss.__lr,    &r->__ss.__sp);
	String_Format1(str, "pc =%x"                      _NL, &r->__ss.__pc);
#elif defined __arm__
	/* /usr/include/mach/arm/_structs.h */
	String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, &r->__ss.__r[0],  &r->__ss.__r[1],  &r->__ss.__r[2]);
	String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, &r->__ss.__r[3],  &r->__ss.__r[4],  &r->__ss.__r[5]);
	String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, &r->__ss.__r[6],  &r->__ss.__r[7],  &r->__ss.__r[8]);
	String_Format3(str, "r9 =%x r10=%x r11=%x" _NL, &r->__ss.__r[9],  &r->__ss.__r[10], &r->__ss.__r[11]);
	String_Format3(str, "r12=%x sp =%x lr =%x" _NL, &r->__ss.__r[12], &r->__ss.__sp,    &r->__ss.__lr);
	String_Format1(str, "pc =%x"               _NL, &r->__ss.__pc);
#elif defined __ppc__ || defined __ppc64__
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->__ss.__r0,   &r->__ss.__r1,  &r->__ss.__r2,  &r->__ss.__r3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->__ss.__r4,   &r->__ss.__r5,  &r->__ss.__r6,  &r->__ss.__r7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->__ss.__r8,   &r->__ss.__r9,  &r->__ss.__r10, &r->__ss.__r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->__ss.__r12,  &r->__ss.__r13, &r->__ss.__r14, &r->__ss.__r15);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->__ss.__r16,  &r->__ss.__r17, &r->__ss.__r18, &r->__ss.__r19);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->__ss.__r20,  &r->__ss.__r21, &r->__ss.__r22, &r->__ss.__r23);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->__ss.__r24,  &r->__ss.__r25, &r->__ss.__r26, &r->__ss.__r27);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->__ss.__r28,  &r->__ss.__r29, &r->__ss.__r30, &r->__ss.__r31);
	String_Format3(str, "pc =%x lr =%x ctr=%x"        _NL, &r->__ss.__srr0, &r->__ss.__lr,  &r->__ss.__ctr);
#else
	#error "Unknown CPU architecture"
#endif
}
#else
/* See /usr/include/mach/i386/thread_status.h (macOS 10.4) */
void CrashHandler_DumpRegisters(void* ctx, cc_string* str) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	String_Format4(str, "eax=%x ebx=%x ecx=%x edx=%x" _NL, &r->ss.eax, &r->ss.ebx, &r->ss.ecx, &r->ss.edx);
	String_Format4(str, "esi=%x edi=%x ebp=%x esp=%x" _NL, &r->ss.esi, &r->ss.edi, &r->ss.ebp, &r->ss.esp);
	String_Format1(str, "eip=%x"                      _NL, &r->ss.eip);
#elif defined __x86_64__
	String_Format4(str, "rax=%x rbx=%x rcx=%x rdx=%x" _NL, &r->ss.rax, &r->ss.rbx, &r->ss.rcx, &r->ss.rdx);
	String_Format4(str, "rsi=%x rdi=%x rbp=%x rsp=%x" _NL, &r->ss.rsi, &r->ss.rdi, &r->ss.rbp, &r->ss.rsp);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->ss.r8,  &r->ss.r9,  &r->ss.r10, &r->ss.r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->ss.r12, &r->ss.r13, &r->ss.r14, &r->ss.r15);
	String_Format1(str, "rip=%x"                      _NL, &r->ss.rip);
#elif defined __ppc__ || defined __ppc64__
	String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, &r->ss.r0,   &r->ss.r1,  &r->ss.r2,  &r->ss.r3);
	String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, &r->ss.r4,   &r->ss.r5,  &r->ss.r6,  &r->ss.r7);
	String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, &r->ss.r8,   &r->ss.r9,  &r->ss.r10, &r->ss.r11);
	String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, &r->ss.r12,  &r->ss.r13, &r->ss.r14, &r->ss.r15);
	String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, &r->ss.r16,  &r->ss.r17, &r->ss.r18, &r->ss.r19);
	String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, &r->ss.r20,  &r->ss.r21, &r->ss.r22, &r->ss.r23);
	String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, &r->ss.r24,  &r->ss.r25, &r->ss.r26, &r->ss.r27);
	String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, &r->ss.r28,  &r->ss.r29, &r->ss.r30, &r->ss.r31);
	String_Format3(str, "pc =%x lr =%x ctr=%x"        _NL, &r->ss.srr0, &r->ss.lr,  &r->ss.ctr);
#else
	#error "Unknown CPU architecture"
#endif
}
#endif
