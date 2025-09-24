#ifndef BR_BEARSSL_STDLIB_H__
#define BR_BEARSSL_STDLIB_H__
/* ==== BEG ClassiCube specific ==== */
static inline size_t br_strlen(const char* a) {
	int i = 0;
	while (*a++) i++;
	return i;
}

static inline size_t br_memcmp(const void* a, const void* b, size_t len) {
	unsigned char* p1 = (unsigned char*)a;
	unsigned char* p2 = (unsigned char*)b;
	size_t i;

	for (i = 0; i < len; i++) 
	{
		if (p1[i] < p2[i]) return -1;
		if (p1[i] > p2[i]) return  1;
	}
	return 0;
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
