#ifndef BR_BEARSSL_STDLIB_H__
#define BR_BEARSSL_STDLIB_H__
/* ==== BEG ClassiCube specific ==== */
static size_t br_strlen(const char* a) {
	int i = 0;
	while (*a++) i++;
	return i;
}

#ifdef CC_BUILD_NOSTDLIB
	extern void* Mem_Copy(void* dst, const void* src,  unsigned size);
	extern void* Mem_Move(void* dst, const void* src,  unsigned size);
	extern void* Mem_Set(void* dst, unsigned char val, unsigned size);

	#define br_memcpy   Mem_Copy
	#define br_memmove  Mem_Move
	#define br_memset   Mem_Set
#else
	#define br_memcpy  memcpy
	#define br_memset  memset
	#define br_memmove memmove
#endif
/* ==== END ClassiCube specific ==== */
#endif
