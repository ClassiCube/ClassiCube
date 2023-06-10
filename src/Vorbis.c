#include "Vorbis.h"
#include "Logger.h"
#include "Platform.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Errors.h"
#include "Stream.h"

/*########################################################################################################################*
*-------------------------------------------------------Ogg stream--------------------------------------------------------*
*#########################################################################################################################*/
#define OGG_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)

static void Ogg_DiscardPacket(struct OggState* ctx) {
	ctx->cur += ctx->left;
	ctx->left = 0;
}

static void Ogg_NextPacket(struct OggState* ctx) {
	cc_uint8 part;
	ctx->left = 0;

	for (; ctx->segmentsRead < ctx->numSegments; ) {
		part = ctx->segments[ctx->segmentsRead++];
		ctx->left += part;
		if (part != 255) break; /* end of this packet */
	}
}

static cc_result Ogg_NextPage(struct OggState* ctx) {
	cc_uint8 header[27];
	struct Stream* source;
	cc_uint32 sig, size;
	cc_result res;
	int i;

	/* OGG page format:
	* header[0]  (4) page signature
	* header[4]  (1) page version
	* header[5]  (1) page flags
	* header[6]  (8) granule position
	* header[14] (4) serial number
	* header[18] (4) page sequence number
	* header[22] (4) page checksum
	* header[26] (1) number of segments
	* [number of segments] number of bytes in each segment
	* [sum of bytes in each segment] page data
	*/
	/* An OGG page is then further split into one or more packets */
	source = ctx->source;
	if ((res = Stream_Read(source, header, sizeof(header)))) return res;

	sig = Stream_GetU32_BE(&header[0]);
	if (sig == OGG_FourCC('I','D','3', 2))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.2 tags header */
	if (sig == OGG_FourCC('I','D','3', 3))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.3 tags header */
	if (sig != OGG_FourCC('O','g','g','S')) return OGG_ERR_INVALID_SIG;
	if (header[4] != 0) return OGG_ERR_VERSION;

	ctx->segmentsRead = 0;
	ctx->numSegments  = header[26];
	if ((res = Stream_Read(source, ctx->segments, ctx->numSegments))) return res;

	size = 0;
	for (i = 0; i < ctx->numSegments; i++) size += ctx->segments[i];
	if ((res = Stream_Read(source, ctx->buffer, size))) return res;

	ctx->cur  = ctx->buffer;
	ctx->last = header[5] & 4;
	Ogg_NextPacket(ctx);
	return 0;
}

static cc_result Ogg_Read(struct OggState* ctx, cc_uint8* data, cc_uint32 count) {
	cc_uint32 left = count;
	cc_result res;
	while (left) {
		if (ctx->left) {
			count = min(left, ctx->left);
			Mem_Copy(data, ctx->cur, count);

			ctx->cur  += count;
			ctx->left -= count;
			left      -= count;
		} else if (ctx->segmentsRead < ctx->numSegments) {
			Ogg_NextPacket(ctx);
		} else {
			if (ctx->last) return ERR_END_OF_STREAM;
			if ((res = Ogg_NextPage(ctx))) return res;
		}
	}
	return 0;
}

static cc_result Ogg_Skip(struct OggState* ctx, cc_uint32 count) {
	cc_uint8 tmp[3584]; /* not quite 4 KB to avoid chkstk call */
	cc_uint32 left = count;
	cc_result res;

	/* TODO: Should Ogg_Read be duplicated here to avoid Mem_Copy call? */
	/* Probably not worth it considering how small comments are */
	while (left) {
		count = min(left, sizeof(tmp));
		if ((res = Ogg_Read(ctx, tmp, count))) return res;
		left -= count;
	}
	return 0;
}

static cc_result Ogg_ReadU8(struct OggState* ctx, cc_uint8* data) {
	/* The fast path below almost always gets used */
	if (!ctx->left) return Ogg_Read(ctx, data, 1);

	*data = *ctx->cur;
	ctx->cur++;
	ctx->left--;
	return 0;
}

static cc_result Ogg_ReadU32(struct OggState* ctx, cc_uint32* value) {
	cc_uint8 data[4]; cc_result res;
	if ((res = Ogg_Read(ctx, data, 4))) return res;
	*value = Stream_GetU32_LE(data); return 0;
}

void Ogg_Init(struct OggState* ctx, struct Stream* source) {
	ctx->cur  = ctx->buffer;
	ctx->left = 0;
	ctx->last = 0;
	ctx->source = source;
	ctx->segmentsRead = 0;
	ctx->numSegments  = 0;
}


/*########################################################################################################################*
*------------------------------------------------------Vorbis utils-------------------------------------------------------*
*#########################################################################################################################*/
#define Vorbis_PushByte(ctx, value) ctx->Bits |= (cc_uint32)(value) << ctx->NumBits; ctx->NumBits += 8;
#define Vorbis_PeekBits(ctx, bits) (ctx->Bits & ((1UL << (bits)) - 1UL))
#define Vorbis_ConsumeBits(ctx, bits) ctx->Bits >>= (bits); ctx->NumBits -= (bits);
/* Aligns bit buffer to be on a byte boundary */
#define Vorbis_AlignBits(ctx) alignSkip = ctx->NumBits & 7; Vorbis_ConsumeBits(ctx, alignSkip);

/* TODO: Make sure this is inlined */
static cc_uint32 Vorbis_ReadBits(struct VorbisState* ctx, cc_uint32 bitsCount) {
	cc_uint8 portion;
	cc_uint32 data;
	cc_result res;

	while (ctx->NumBits < bitsCount) {
		res = Ogg_ReadU8(ctx->source, &portion);
		if (res) { Logger_Abort2(res, "Failed to read byte for vorbis"); }
		Vorbis_PushByte(ctx, portion);
	}

	data = Vorbis_PeekBits(ctx, bitsCount); Vorbis_ConsumeBits(ctx, bitsCount);
	return data;
}

static cc_result Vorbis_TryReadBits(struct VorbisState* ctx, cc_uint32 bitsCount, cc_uint32* data) {
	cc_uint8 portion;
	cc_result res;

	while (ctx->NumBits < bitsCount) {
		res = Ogg_ReadU8(ctx->source, &portion);
		if (res) return res;
		Vorbis_PushByte(ctx, portion);
	}

	*data = Vorbis_PeekBits(ctx, bitsCount); Vorbis_ConsumeBits(ctx, bitsCount);
	return 0;
}

static cc_uint32 Vorbis_ReadBit(struct VorbisState* ctx) {
	cc_uint8 portion;
	cc_uint32 data;
	cc_result res;

	if (!ctx->NumBits) {
		res = Ogg_ReadU8(ctx->source, &portion);
		if (res) { Logger_Abort2(res, "Failed to read byte for vorbis"); }
		Vorbis_PushByte(ctx, portion);
	}

	data = Vorbis_PeekBits(ctx, 1); Vorbis_ConsumeBits(ctx, 1);
	return data;
}


static int iLog(int x) {
	int bits = 0;
	while (x > 0) { bits++; x >>= 1; }
	return bits;
}

static float float32_unpack(struct VorbisState* ctx) {
	/* ReadBits can't reliably read over 24 bits */
	cc_uint32 lo = Vorbis_ReadBits(ctx, 16);
	cc_uint32 hi = Vorbis_ReadBits(ctx, 16);
	cc_uint32 x = (hi << 16) | lo;

	cc_int32 mantissa  =  x & 0x1fffff;
	cc_uint32 exponent = (x & 0x7fe00000) >> 21;
	if (x & 0x80000000UL) mantissa = -mantissa;

	return (float)Math_ldexp(mantissa, (int)exponent - 788);
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis codebooks-----------------------------------------------------*
*#########################################################################################################################*/
#define CODEBOOK_SYNC 0x564342
struct Codebook {
	cc_uint32 dimensions, entries, totalCodewords;
	cc_uint32* codewords;
	cc_uint32* values;
	cc_uint32 numCodewords[33]; /* number of codewords of bit length i */
	/* vector quantisation values */
	float minValue, deltaValue;
	cc_uint32 sequenceP, lookupType, lookupValues;
	cc_uint16* multiplicands;
};

static void Codebook_Free(struct Codebook* c) {
	Mem_Free(c->codewords);
	Mem_Free(c->values);
	Mem_Free(c->multiplicands);
}

static cc_uint32 Codebook_Pow(cc_uint32 base, cc_uint32 exp) {
	cc_uint32 result = 1; /* exponentiation by squaring */
	while (exp) {
		if (exp & 1) result *= base;
		exp >>= 1;
		base *= base;
	}
	return result;
}

static cc_uint32 Codebook_Lookup1Values(cc_uint32 entries, cc_uint32 dimensions) {
	cc_uint32 i, pow, next;
	/* the greatest integer value for which [value] to the power of [dimensions] is less than or equal to [entries] */
	/* TODO: verify this */
	for (i = 1; ; i++) {
		pow  = Codebook_Pow(i,     dimensions);
		next = Codebook_Pow(i + 1, dimensions);

		if (next < pow)     return i; /* overflow */
		if (pow == entries) return i;
		if (next > entries) return i;
	}
	return 0;
}

static cc_bool Codebook_CalcCodewords(struct Codebook* c, cc_uint8* len) {
	/* This is taken from stb_vorbis.c because I gave up trying */
	cc_uint32 i, depth;
	cc_uint32 root, codeword;
	cc_uint32 next_codewords[33] = { 0 };
	int offset;
	int len_offsets[33];

	c->codewords = (cc_uint32*)Mem_Alloc(c->totalCodewords, 4, "codewords");
	c->values    = (cc_uint32*)Mem_Alloc(c->totalCodewords, 4, "values");

	/* Codeword entries are ordered by length */
	offset = 0;
	for (i = 0; i < Array_Elems(len_offsets); i++) {
		len_offsets[i] = offset;
		offset += c->numCodewords[i];
	}

	/* add codeword 0 to tree */
	for (i = 0; i < c->entries; i++) {
		if (!len[i]) continue;
		offset = len_offsets[len[i]];

		c->codewords[offset] = 0;
		c->values[offset]    = i;

		len_offsets[len[i]]++;
		break;
	}

	/* set codewords that new nodes can start from */
	for (depth = 1; depth <= len[i]; depth++) {
		next_codewords[depth] = 1U << (32 - depth);
	}

	i++; /* first codeword was already handled */
	for (; i < c->entries; i++) {
		root = len[i];
		if (!root) continue;
		offset = len_offsets[len[i]];

		/* per spec, find lowest possible value (leftmost) */
		while (root && next_codewords[root] == 0) root--;
		if (root == 0) return false;

		codeword = next_codewords[root];
		next_codewords[root] = 0;

		c->codewords[offset] = codeword;
		c->values[offset]    = i;

		for (depth = len[i]; depth > root; depth--) {
			next_codewords[depth] = codeword + (1U << (32 - depth));
		}

		len_offsets[len[i]]++;
	}
	return true;
}

static cc_result Codebook_DecodeSetup(struct VorbisState* ctx, struct Codebook* c) {
	cc_uint32 sync;
	cc_uint8* codewordLens;
	int i, entry;
	int sparse, len;
	int runBits, runLen;
	int valueBits;
	cc_uint32 lookupValues;

	sync = Vorbis_ReadBits(ctx, 24);
	if (sync != CODEBOOK_SYNC) return VORBIS_ERR_CODEBOOK_SYNC;
	c->dimensions = Vorbis_ReadBits(ctx, 16);
	c->entries    = Vorbis_ReadBits(ctx, 24);

	codewordLens = (cc_uint8*)Mem_Alloc(c->entries, 1, "raw codeword lens");
	for (i = 0; i < Array_Elems(c->numCodewords); i++) {
		c->numCodewords[i] = 0;
	}

	/* ordered entries flag */
	if (!Vorbis_ReadBit(ctx)) {
		sparse = Vorbis_ReadBit(ctx);
		entry  = 0;
		for (i = 0; i < c->entries; i++) {
			/* sparse trees may not have all entries */
			if (sparse && !Vorbis_ReadBit(ctx)){
				codewordLens[i] = 0;
				continue; /* unused entry */
			}

			len = Vorbis_ReadBits(ctx, 5) + 1;
			codewordLens[i] = len;
			c->numCodewords[len]++;
			entry++;
		}
	} else {
		len = Vorbis_ReadBits(ctx, 5) + 1;
		for (entry = 0; entry < c->entries;) {
			runBits = iLog(c->entries - entry);
			runLen  = Vorbis_ReadBits(ctx, runBits);

			/* handle corrupted ogg files */
			if (entry + runLen > c->entries) return VORBIS_ERR_CODEBOOK_ENTRY;

			for (i = 0; i < runLen; i++) { codewordLens[entry++] = len; }
			c->numCodewords[len++] = runLen;
		}
	}

	c->totalCodewords = entry;
	Codebook_CalcCodewords(c, codewordLens);
	Mem_Free(codewordLens);

	c->lookupType    = Vorbis_ReadBits(ctx, 4);
	c->multiplicands = NULL;
	if (c->lookupType == 0) return 0;
	if (c->lookupType > 2)  return VORBIS_ERR_CODEBOOK_LOOKUP;

	c->minValue   = float32_unpack(ctx);
	c->deltaValue = float32_unpack(ctx);
	valueBits     = Vorbis_ReadBits(ctx, 4) + 1;
	c->sequenceP  = Vorbis_ReadBit(ctx);

	if (c->lookupType == 1) {
		lookupValues = Codebook_Lookup1Values(c->entries, c->dimensions);
	} else {
		lookupValues = c->entries * c->dimensions;
	}
	c->lookupValues = lookupValues;

	c->multiplicands = (cc_uint16*)Mem_Alloc(lookupValues, 2, "multiplicands");
	for (i = 0; i < lookupValues; i++) {
		c->multiplicands[i] = Vorbis_ReadBits(ctx, valueBits);
	}
	return 0;
}

static cc_uint32 Codebook_DecodeScalar(struct VorbisState* ctx, struct Codebook* c) {
	cc_uint32 codeword = 0, shift = 31, depth, i;
	cc_uint32* codewords = c->codewords;
	cc_uint32* values    = c->values;

	/* TODO: This is so massively slow */
	for (depth = 1; depth <= 32; depth++, shift--) {
		codeword |= Vorbis_ReadBit(ctx) << shift;

		for (i = 0; i < c->numCodewords[depth]; i++) {
			if (codeword != codewords[i]) continue;
			return values[i];
		}

		codewords += c->numCodewords[depth];
		values    += c->numCodewords[depth];
	}
	Logger_Abort("Invalid huffman code");
	return -1;
}

static void Codebook_DecodeVectors(struct VorbisState* ctx, struct Codebook* c, float* v, int step) {
	cc_uint32 lookupOffset = Codebook_DecodeScalar(ctx, c);
	float last = 0.0f, value;
	cc_uint32 i, offset;

	if (c->lookupType == 1) {		
		cc_uint32 indexDivisor = 1;
		for (i = 0; i < c->dimensions; i++, v += step) {
			offset = (lookupOffset / indexDivisor) % c->lookupValues;
			value  = c->multiplicands[offset] * c->deltaValue + c->minValue + last;

			*v += value;
			if (c->sequenceP) last = value;
			indexDivisor *= c->lookupValues;
		}
	} else if (c->lookupType == 2) {
		offset = lookupOffset * c->dimensions;
		for (i = 0; i < c->dimensions; i++, offset++, v += step) {
			value  = c->multiplicands[offset] * c->deltaValue + c->minValue + last;

			*v += value;
			if (c->sequenceP) last = value;
		}
	} else {
		Logger_Abort("Invalid huffman code");
	}
}

/*########################################################################################################################*
*-----------------------------------------------------Vorbis floors-------------------------------------------------------*
*#########################################################################################################################*/
#define FLOOR_MAX_PARTITIONS 32
#define FLOOR_MAX_CLASSES 16
#define FLOOR_MAX_VALUES (FLOOR_MAX_PARTITIONS * 8 + 2)
struct Floor {
	cc_uint8 partitions, multiplier; int range, values;
	cc_uint8 partitionClasses[FLOOR_MAX_PARTITIONS];
	cc_uint8 classDimensions[FLOOR_MAX_CLASSES];
	cc_uint8 classSubClasses[FLOOR_MAX_CLASSES];
	cc_uint8 classMasterbooks[FLOOR_MAX_CLASSES];
	cc_int16 subclassBooks[FLOOR_MAX_CLASSES][8];
	cc_int16  xList[FLOOR_MAX_VALUES];
	cc_uint16 listOrder[FLOOR_MAX_VALUES];
	cc_int32  yList[VORBIS_MAX_CHANS][FLOOR_MAX_VALUES];
};

static const float floor1_inverse_dB_table[256] = {
	1.0649863e-07f, 1.1341951e-07f, 1.2079015e-07f, 1.2863978e-07f, 1.3699951e-07f, 1.4590251e-07f, 1.5538408e-07f, 1.6548181e-07f,
	1.7623575e-07f, 1.8768855e-07f, 1.9988561e-07f, 2.1287530e-07f, 2.2670913e-07f, 2.4144197e-07f, 2.5713223e-07f, 2.7384213e-07f,
	2.9163793e-07f, 3.1059021e-07f, 3.3077411e-07f, 3.5226968e-07f, 3.7516214e-07f, 3.9954229e-07f, 4.2550680e-07f, 4.5315863e-07f,
	4.8260743e-07f, 5.1396998e-07f, 5.4737065e-07f, 5.8294187e-07f, 6.2082472e-07f, 6.6116941e-07f, 7.0413592e-07f, 7.4989464e-07f,
	7.9862701e-07f, 8.5052630e-07f, 9.0579828e-07f, 9.6466216e-07f, 1.0273513e-06f, 1.0941144e-06f, 1.1652161e-06f, 1.2409384e-06f,
	1.3215816e-06f, 1.4074654e-06f, 1.4989305e-06f, 1.5963394e-06f, 1.7000785e-06f, 1.8105592e-06f, 1.9282195e-06f, 2.0535261e-06f,
	2.1869758e-06f, 2.3290978e-06f, 2.4804557e-06f, 2.6416497e-06f, 2.8133190e-06f, 2.9961443e-06f, 3.1908506e-06f, 3.3982101e-06f,
	3.6190449e-06f, 3.8542308e-06f, 4.1047004e-06f, 4.3714470e-06f, 4.6555282e-06f, 4.9580707e-06f, 5.2802740e-06f, 5.6234160e-06f,
	5.9888572e-06f, 6.3780469e-06f, 6.7925283e-06f, 7.2339451e-06f, 7.7040476e-06f, 8.2047000e-06f, 8.7378876e-06f, 9.3057248e-06f,
	9.9104632e-06f, 1.0554501e-05f, 1.1240392e-05f, 1.1970856e-05f, 1.2748789e-05f, 1.3577278e-05f, 1.4459606e-05f, 1.5399272e-05f,
	1.6400004e-05f, 1.7465768e-05f, 1.8600792e-05f, 1.9809576e-05f, 2.1096914e-05f, 2.2467911e-05f, 2.3928002e-05f, 2.5482978e-05f,
	2.7139006e-05f, 2.8902651e-05f, 3.0780908e-05f, 3.2781225e-05f, 3.4911534e-05f, 3.7180282e-05f, 3.9596466e-05f, 4.2169667e-05f,
	4.4910090e-05f, 4.7828601e-05f, 5.0936773e-05f, 5.4246931e-05f, 5.7772202e-05f, 6.1526565e-05f, 6.5524908e-05f, 6.9783085e-05f,
	7.4317983e-05f, 7.9147585e-05f, 8.4291040e-05f, 8.9768747e-05f, 9.5602426e-05f, 0.00010181521f, 0.00010843174f, 0.00011547824f,
	0.00012298267f, 0.00013097477f, 0.00013948625f, 0.00014855085f, 0.00015820453f, 0.00016848555f, 0.00017943469f, 0.00019109536f,
	0.00020351382f, 0.00021673929f, 0.00023082423f, 0.00024582449f, 0.00026179955f, 0.00027881276f, 0.00029693158f, 0.00031622787f,
	0.00033677814f, 0.00035866388f, 0.00038197188f, 0.00040679456f, 0.00043323036f, 0.00046138411f, 0.00049136745f, 0.00052329927f,
	0.00055730621f, 0.00059352311f, 0.00063209358f, 0.00067317058f, 0.00071691700f, 0.00076350630f, 0.00081312324f, 0.00086596457f,
	0.00092223983f, 0.00098217216f, 0.0010459992f,  0.0011139742f,  0.0011863665f,  0.0012634633f,  0.0013455702f,  0.0014330129f,
	0.0015261382f,  0.0016253153f,  0.0017309374f,  0.0018434235f,  0.0019632195f,  0.0020908006f,  0.0022266726f,  0.0023713743f,
	0.0025254795f,  0.0026895994f,  0.0028643847f,  0.0030505286f,  0.0032487691f,  0.0034598925f,  0.0036847358f,  0.0039241906f,
	0.0041792066f,  0.0044507950f,  0.0047400328f,  0.0050480668f,  0.0053761186f,  0.0057254891f,  0.0060975636f,  0.0064938176f,
	0.0069158225f,  0.0073652516f,  0.0078438871f,  0.0083536271f,  0.0088964928f,  0.009474637f,   0.010090352f,   0.010746080f,
	0.011444421f,   0.012188144f,   0.012980198f,   0.013823725f,   0.014722068f,   0.015678791f,   0.016697687f,   0.017782797f,
	0.018938423f,   0.020169149f,   0.021479854f,   0.022875735f,   0.024362330f,   0.025945531f,   0.027631618f,   0.029427276f,
	0.031339626f,   0.033376252f,   0.035545228f,   0.037855157f,   0.040315199f,   0.042935108f,   0.045725273f,   0.048696758f,
	0.051861348f,   0.055231591f,   0.058820850f,   0.062643361f,   0.066714279f,   0.071049749f,   0.075666962f,   0.080584227f,
	0.085821044f,   0.091398179f,   0.097337747f,   0.10366330f,    0.11039993f,    0.11757434f,    0.12521498f,    0.13335215f,
	0.14201813f,    0.15124727f,    0.16107617f,    0.17154380f,    0.18269168f,    0.19456402f,    0.20720788f,    0.22067342f,
	0.23501402f,    0.25028656f,    0.26655159f,    0.28387361f,    0.30232132f,    0.32196786f,    0.34289114f,    0.36517414f,
	0.38890521f,    0.41417847f,    0.44109412f,    0.46975890f,    0.50028648f,    0.53279791f,    0.56742212f,    0.60429640f,
	0.64356699f,    0.68538959f,    0.72993007f,    0.77736504f,    0.82788260f,    0.88168307f,    0.9389798f,     1.00000000f,
};

/* TODO: Make this thread safe */
static cc_int16* tmp_xlist;
static cc_uint16* tmp_order;
static void Floor_SortXList(int left, int right) {
	cc_uint16* values = tmp_order; cc_uint16 value;
	cc_int16* keys = tmp_xlist;    cc_int16 key;

	while (left < right) {
		int i = left, j = right;
		cc_int16 pivot = keys[(i + j) >> 1];

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Floor_SortXList)
	}
}

static cc_result Floor_DecodeSetup(struct VorbisState* ctx, struct Floor* f) {
	static const short ranges[4] = { 256, 128, 84, 64 };
	int i, j, idx, maxClass;
	int rangeBits, classNum;
	cc_int16 xlist_sorted[FLOOR_MAX_VALUES];

	f->partitions = Vorbis_ReadBits(ctx, 5);
	maxClass = -1;
	for (i = 0; i < f->partitions; i++) {
		f->partitionClasses[i] = Vorbis_ReadBits(ctx, 4);
		maxClass = max(maxClass, f->partitionClasses[i]);
	}

	for (i = 0; i <= maxClass; i++) {
		f->classDimensions[i] = Vorbis_ReadBits(ctx, 3) + 1;
		f->classSubClasses[i] = Vorbis_ReadBits(ctx, 2);

		if (f->classSubClasses[i]) {
			f->classMasterbooks[i] = Vorbis_ReadBits(ctx, 8);
		}
		for (j = 0; j < (1 << f->classSubClasses[i]); j++) {
			f->subclassBooks[i][j] = (cc_int16)Vorbis_ReadBits(ctx, 8) - 1;
		}
	}

	f->multiplier = Vorbis_ReadBits(ctx, 2) + 1;
	f->range      = ranges[f->multiplier - 1];
	rangeBits     = Vorbis_ReadBits(ctx, 4);

	f->xList[0] = 0;
	f->xList[1] = 1 << rangeBits;
	for (i = 0, idx = 2; i < f->partitions; i++) {
		classNum = f->partitionClasses[i];

		for (j = 0; j < f->classDimensions[classNum]; j++) {
			f->xList[idx++] = Vorbis_ReadBits(ctx, rangeBits);
		}
	}
	f->values = idx;

	/* sort X list for curve computation later */
	Mem_Copy(xlist_sorted, f->xList, idx * 2);
	for (i = 0; i < idx; i++) { f->listOrder[i] = i; }

	tmp_xlist = xlist_sorted; 
	tmp_order = f->listOrder;
	Floor_SortXList(0, idx - 1);
	return 0;
}

static cc_bool Floor_DecodeFrame(struct VorbisState* ctx, struct Floor* f, int ch) {
	cc_int32* yList;
	int i, j, idx, rangeBits;
	cc_uint8 klass, cdim, cbits;
	int bookNum;
	cc_uint32 csub, cval;

	/* does this frame have any energy */
	if (!Vorbis_ReadBit(ctx)) return false;
	yList = f->yList[ch];

	rangeBits = iLog(f->range - 1);
	yList[0]  = Vorbis_ReadBits(ctx, rangeBits);
	yList[1]  = Vorbis_ReadBits(ctx, rangeBits);

	for (i = 0, idx = 2; i < f->partitions; i++) {
		klass = f->partitionClasses[i];
		cdim  = f->classDimensions[klass];
		cbits = f->classSubClasses[klass];

		csub = (1 << cbits) - 1;
		cval = 0;
		if (cbits) {
			bookNum = f->classMasterbooks[klass];
			cval = Codebook_DecodeScalar(ctx, &ctx->codebooks[bookNum]);
		}

		for (j = 0; j < cdim; j++) {
			bookNum = f->subclassBooks[klass][cval & csub];
			cval >>= cbits;

			if (bookNum >= 0) {
				yList[idx + j] = Codebook_DecodeScalar(ctx, &ctx->codebooks[bookNum]);
			} else {
				yList[idx + j] = 0;
			}
		}
		idx += cdim;
	}
	return true;
}

static int Floor_RenderPoint(int x0, int y0, int x1, int y1, int X) {
	int dy  = y1 - y0, adx = x1 - x0;
	int ady = Math_AbsI(dy);
	int err = ady * (X - x0);
	int off = err / adx;

	if (dy < 0) {
		return y0 - off;
	} else {
		return y0 + off;
	}
}

static void Floor_RenderLine(int x0, int y0, int x1, int y1, float* data) {
	int dy   = y1 - y0, adx = x1 - x0;
	int ady  = Math_AbsI(dy);
	int base = dy / adx, sy;
	int x    = x0, y = y0, err = 0;

	if (dy < 0) {
		sy = base - 1;
	} else {
		sy = base + 1;
	}

	ady = ady - Math_AbsI(base) * adx;
	data[x] *= floor1_inverse_dB_table[y];

	for (x = x0 + 1; x < x1; x++) {
		err = err + ady;
		if (err >= adx) {
			err = err - adx;
			y = y + sy;
		} else {
			y = y + base;
		}
		data[x] *= floor1_inverse_dB_table[y];
	}
}

static int low_neighbor(cc_int16* v, int x) {
	int n = 0, i, max = Int32_MinValue;
	for (i = 0; i < x; i++) {
		if (v[i] < v[x] && v[i] > max) { n = i; max = v[i]; }
	}
	return n;
}

static int high_neighbor(cc_int16* v, int x) {
	int n = 0, i, min = Int32_MaxValue;
	for (i = 0; i < x; i++) {
		if (v[i] > v[x] && v[i] < min) { n = i; min = v[i]; }
	}
	return n;
}

static void Floor_Synthesis(struct VorbisState* ctx, struct Floor* f, int ch) {
	/* amplitude arrays */
	cc_int32 YFinal[FLOOR_MAX_VALUES];
	cc_bool Step2[FLOOR_MAX_VALUES];
	cc_int32* yList;
	float* data;
	/* amplitude variables */
	int lo_offset, hi_offset, predicted;
	int val, highroom, lowroom, room;
	int i;
	/* curve variables */
	int lx, hx, ly, hy;
	int rawI;
	float value;

	/* amplitude value synthesis */
	yList = f->yList[ch];
	data  = ctx->curOutput[ch];

	Step2[0]  = true;
	Step2[1]  = true;
	YFinal[0] = yList[0];
	YFinal[1] = yList[1];

	for (i = 2; i < f->values; i++) {
		lo_offset = low_neighbor(f->xList, i);
		hi_offset = high_neighbor(f->xList, i);
		predicted = Floor_RenderPoint(f->xList[lo_offset], YFinal[lo_offset],
									  f->xList[hi_offset], YFinal[hi_offset], f->xList[i]);

		val      = yList[i];
		highroom = f->range - predicted;
		lowroom  = predicted;

		if (highroom < lowroom) {
			room = highroom * 2;
		} else {
			room = lowroom * 2;
		}

		if (val) {
			Step2[lo_offset] = true;
			Step2[hi_offset] = true;
			Step2[i] = true;

			if (val >= room) {
				if (highroom > lowroom) {
					YFinal[i] = val - lowroom + predicted;
				} else {
					YFinal[i] = predicted - val + highroom - 1;
				}
			} else {
				if (val & 1) {
					YFinal[i] = predicted - (val + 1) / 2;
				} else {
					YFinal[i] = predicted + val / 2;
				}
			}
		} else {
			Step2[i]  = false;
			YFinal[i] = predicted;
		}
	}

	/* curve synthesis */ 
	lx = 0; ly = YFinal[f->listOrder[0]] * f->multiplier; 
	hx = 0; hy = ly;

	for (rawI = 1; rawI < f->values; rawI++) {
		i = f->listOrder[rawI];
		if (!Step2[i]) continue;

		hx = f->xList[i]; hy = YFinal[i] * f->multiplier;
		if (lx < hx) {
			Floor_RenderLine(lx, ly, min(hx, ctx->dataSize), hy, data);
		}
		lx = hx; ly = hy;
	}

	/* fill remainder of floor with a flat line */
	/* TODO: Is this right? should hy be 0, if Step2 is false for all */
	if (hx >= ctx->dataSize) return;
	lx = hx; hx = ctx->dataSize;

	value = floor1_inverse_dB_table[hy];
	for (; lx < hx; lx++) { data[lx] *= value; }
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis residues------------------------------------------------------*
*#########################################################################################################################*/
#define RESIDUE_MAX_CLASSIFICATIONS 65
struct Residue {
	cc_uint8  type, classifications, classbook;
	cc_uint32 begin, end, partitionSize;
	cc_uint8  cascade[RESIDUE_MAX_CLASSIFICATIONS];
	cc_int16  books[RESIDUE_MAX_CLASSIFICATIONS][8];
};

static cc_result Residue_DecodeSetup(struct VorbisState* ctx, struct Residue* r, int type) {
	cc_int16 codebook;
	int i, j;

	r->type  = (cc_uint8)type;
	r->begin = Vorbis_ReadBits(ctx, 24);
	r->end   = Vorbis_ReadBits(ctx, 24);
	r->partitionSize   = Vorbis_ReadBits(ctx, 24) + 1;
	r->classifications = Vorbis_ReadBits(ctx, 6)  + 1;
	r->classbook       = Vorbis_ReadBits(ctx, 8);

	for (i = 0; i < r->classifications; i++) {
		r->cascade[i] = Vorbis_ReadBits(ctx, 3);
		if (!Vorbis_ReadBit(ctx)) continue;
		r->cascade[i] |= Vorbis_ReadBits(ctx, 5) << 3;
	}

	for (i = 0; i < r->classifications; i++) {
		for (j = 0; j < 8; j++) {
			codebook = -1;

			if (r->cascade[i] & (1 << j)) {
				codebook = Vorbis_ReadBits(ctx, 8);
			}
			r->books[i][j] = codebook;
		}
	}
	return 0;
}

static void Residue_DecodeCore(struct VorbisState* ctx, struct Residue* r, cc_uint32 size, int ch, cc_bool* doNotDecode, float** data) {
	struct Codebook* classbook;
	cc_uint32 residueBeg, residueEnd;
	cc_uint32 classwordsPerCodeword;
	cc_uint32 nToRead, partitionsToRead;
	int pass, i, j, k;

	/* classification variables */
	cc_uint8* classifications[VORBIS_MAX_CHANS];
	cc_uint8* classifications_raw;
	cc_uint32 temp;

	/* partition variables */
	struct Codebook* c;
	float* v;
	cc_uint32 offset;
	cc_uint8 klass;
	cc_int16 book;

	/* per spec, ensure decoded bounds are actually in size */
	residueBeg = min(r->begin, size);
	residueEnd = min(r->end,   size);
	classbook = &ctx->codebooks[r->classbook];

	classwordsPerCodeword = classbook->dimensions;
	nToRead = residueEnd - residueBeg;
	partitionsToRead = nToRead / r->partitionSize;

	/* first half of temp array is used by residue type 2 for storing temp interleaved data */
	classifications_raw = ((cc_uint8*)ctx->temp) + (ctx->dataSize * ctx->channels * 5);
	for (i = 0; i < ch; i++) {
		/* add a bit of space in case classwordsPerCodeword is > partitionsToRead*/
		classifications[i] = classifications_raw + i * (partitionsToRead + 64);
	}

	if (nToRead == 0) return;
	for (pass = 0; pass < 8; pass++) {
		cc_uint32 partitionCount = 0;
		while (partitionCount < partitionsToRead) {

			/* read classifications in pass 0 */
			if (pass == 0) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;

					temp = Codebook_DecodeScalar(ctx, classbook);
					for (i = classwordsPerCodeword - 1; i >= 0; i--) {
						classifications[j][i + partitionCount] = temp % r->classifications;
						temp /= r->classifications;
					}
				}
			}

			for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
				for (j = 0; j < ch; j++) {
					if (doNotDecode[j]) continue;

					klass = classifications[j][partitionCount];
					book  = r->books[klass][pass];
					if (book < 0) continue;

					offset = residueBeg + partitionCount * r->partitionSize;
					v = data[j] + offset;
					c = &ctx->codebooks[book];

					if (r->type == 0) {
						int step = r->partitionSize / c->dimensions;
						for (k = 0; k < step; k++) {
							Codebook_DecodeVectors(ctx, c, v, step); v++;
						}
					} else {
						for (k = 0; k < r->partitionSize; k += c->dimensions) {
							Codebook_DecodeVectors(ctx, c, v, 1); v += c->dimensions;
						}
					}
				}
				partitionCount++;
			}
		}
	}
}

static void Residue_DecodeFrame(struct VorbisState* ctx, struct Residue* r, int ch, cc_bool* doNotDecode, float** data) {
	cc_uint32 size = ctx->dataSize;
	float* interleaved;
	cc_bool decodeAny;
	int i, j;

	if (r->type == 2) {
		decodeAny = false;

		/* type 2 decodes all channel vectors, if at least 1 channel to decode */
		for (i = 0; i < ch; i++) {
			if (!doNotDecode[i]) decodeAny = true;
		}
		if (!decodeAny) return;
		decodeAny = false; /* because DecodeCore expects this to be 'false' for 'do not decode' */

		interleaved = ctx->temp;
		/* TODO: avoid using ctx->temp and deinterleaving at all */
		/* TODO: avoid setting memory to 0 here */
		Mem_Set(interleaved, 0, ctx->dataSize * ctx->channels * sizeof(float));
		Residue_DecodeCore(ctx, r, size * ch, 1, &decodeAny, &interleaved);

		/* deinterleave type 2 output */	
		for (i = 0; i < size; i++) {
			for (j = 0; j < ch; j++) {
				data[j][i] = interleaved[i * ch + j];
			}
		}
	} else {
		Residue_DecodeCore(ctx, r, size, ch, doNotDecode, data);
	}
}


/*########################################################################################################################*
*----------------------------------------------------Vorbis mappings------------------------------------------------------*
*#########################################################################################################################*/
#define MAPPING_MAX_COUPLINGS 256
#define MAPPING_MAX_SUBMAPS 15
struct Mapping {
	cc_uint8 couplingSteps, submaps;
	cc_uint8 mux[VORBIS_MAX_CHANS];
	cc_uint8 floorIdx[MAPPING_MAX_SUBMAPS];
	cc_uint8 residueIdx[MAPPING_MAX_SUBMAPS];
	cc_uint8 magnitude[MAPPING_MAX_COUPLINGS];
	cc_uint8 angle[MAPPING_MAX_COUPLINGS];
};

static cc_result Mapping_DecodeSetup(struct VorbisState* ctx, struct Mapping* m) {
	int i, submaps, reserved;
	int couplingSteps, couplingBits;

	submaps = 1;
	if (Vorbis_ReadBit(ctx)) {
		submaps = Vorbis_ReadBits(ctx, 4) + 1;
	}

	couplingSteps = 0;
	if (Vorbis_ReadBit(ctx)) {
		couplingSteps = Vorbis_ReadBits(ctx, 8) + 1;
		/* TODO: How big can couplingSteps ever really get in practice? */
		couplingBits  = iLog(ctx->channels - 1);

		for (i = 0; i < couplingSteps; i++) {
			m->magnitude[i] = Vorbis_ReadBits(ctx, couplingBits);
			m->angle[i]     = Vorbis_ReadBits(ctx, couplingBits);
			if (m->magnitude[i] == m->angle[i]) return VORBIS_ERR_MAPPING_CHANS;
		}
	}

	reserved = Vorbis_ReadBits(ctx, 2);
	if (reserved != 0) return VORBIS_ERR_MAPPING_RESERVED;
	m->submaps = submaps;
	m->couplingSteps = couplingSteps;

	if (submaps > 1) {
		for (i = 0; i < ctx->channels; i++) {
			m->mux[i] = Vorbis_ReadBits(ctx, 4);
		}
	} else {
		for (i = 0; i < ctx->channels; i++) {
			m->mux[i] = 0;
		}
	}

	for (i = 0; i < submaps; i++) {
		Vorbis_ReadBits(ctx, 8); /* time value */
		m->floorIdx[i]   = Vorbis_ReadBits(ctx, 8);
		m->residueIdx[i] = Vorbis_ReadBits(ctx, 8);
	}
	return 0;
}


/*########################################################################################################################*
*------------------------------------------------------imdct impl---------------------------------------------------------*
*#########################################################################################################################*/
#define PI MATH_PI
static cc_uint32 Vorbis_ReverseBits(cc_uint32 v) {
	v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
	v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
	v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
	v = (v >> 16) | (v << 16);
	return v;
}

void imdct_init(struct imdct_state* state, int n) {
	int k, k2, n4 = n >> 2, n8 = n >> 3, log2_n;
	float *A = state->a, *B = state->b, *C = state->c;
	cc_uint32* reversed;

	log2_n   = Math_Log2(n);
	reversed = state->reversed;
	state->n = n; state->log2_n = log2_n;

	/* setup twiddle factors */
	for (k = 0, k2 = 0; k < n4; k++, k2 += 2) {
		A[k2]   =  (float)Math_Cos((4*k * PI) / n);
		A[k2+1] = -(float)Math_Sin((4*k * PI) / n);
		B[k2]   =  (float)Math_Cos(((k2+1) * PI) / (2*n));
		B[k2+1] =  (float)Math_Sin(((k2+1) * PI) / (2*n));
	}
	for (k = 0, k2 = 0; k < n8; k++, k2 += 2) {
		C[k2]   =  (float)Math_Cos(((k2+1) * (2*PI)) / n);
		C[k2+1] = -(float)Math_Sin(((k2+1) * (2*PI)) / n);
	}

	for (k = 0; k < n8; k++) {
		reversed[k] = Vorbis_ReverseBits(k) >> (32-log2_n+3);
	}
}

void imdct_calc(float* in, float* out, struct imdct_state* state) {
	int k, k2, k4, n = state->n;
	int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, n3_4 = n - n4;
	int l, log2_n;
	cc_uint32* reversed;
	
	/* Optimised algorithm from "The use of multirate filter banks for coding of high quality digital audio" */
	/* Uses a few fixes for the paper noted at http://www.nothings.org/stb_vorbis/mdct_01.txt */
	float *A = state->a, *B = state->b, *C = state->c;

	float u[VORBIS_MAX_BLOCK_SIZE / 2];
	float w[VORBIS_MAX_BLOCK_SIZE / 2];
	float e_1, e_2, f_1, f_2;
	float g_1, g_2, h_1, h_2;
	float x_1, x_2, y_1, y_2;


	/* spectral coefficients, step 1, step 2 */ /* TODO avoid k */
	for (k = 0, k2 = 0, k4 = 0; k < n8; k++, k2 += 2, k4 += 4) {
		e_1 = -in[k4+3];   e_2 = -in[k4+1];
		g_1 = e_1 * A[n2-1-k2] + e_2 * A[n2-2-k2];
		g_2 = e_1 * A[n2-2-k2] - e_2 * A[n2-1-k2];

		f_1 = in[n2-4-k4]; f_2 = in[n2-2-k4];
		h_2 = f_1 * A[n4-2-k2] - f_2 * A[n4-1-k2];
		h_1 = f_1 * A[n4-1-k2] + f_2 * A[n4-2-k2];

		w[n4+1+k2] = h_2 + g_2;
		w[n4+0+k2] = h_1 + g_1;

		w[k2+1] = (h_2 - g_2) * A[n2-4-k4] - (h_1 - g_1) * A[n2-3-k4];
		w[k2+0] = (h_1 - g_1) * A[n2-4-k4] + (h_2 - g_2) * A[n2-3-k4];
	}

	/* step 3 */
	log2_n = state->log2_n;
	for (l = 0; l <= log2_n - 4; l++) {
		int k0 = n >> (l+3), k1 = 1 << (l+3);
		int r, r2, rMax = n >> (l+4), s2, s2Max = 1 << (l+2);

		for (r = 0, r2 = 0; r < rMax; r++, r2 += 2) {
			for (s2 = 0; s2 < s2Max; s2 += 2) {
				e_1 = w[n2-1-k0*s2-r2];     
				e_2 = w[n2-2-k0*s2-r2];
				f_1 = w[n2-1-k0*(s2+1)-r2]; 
				f_2 = w[n2-2-k0*(s2+1)-r2];

				u[n2-1-k0*s2-r2]     = e_1 + f_1;
				u[n2-2-k0*s2-r2]     = e_2 + f_2;
				u[n2-1-k0*(s2+1)-r2] = (e_1 - f_1) * A[r*k1] - (e_2 - f_2) * A[r*k1+1];
				u[n2-2-k0*(s2+1)-r2] = (e_2 - f_2) * A[r*k1] + (e_1 - f_1) * A[r*k1+1];
			}
		}

		/* TODO: eliminate this, do w/u in-place */
		/* TODO: dynamically allocate mem for imdct */
		if (l+1 <= log2_n - 4) {
			Mem_Copy(w, u, n2 * sizeof(float));
		}
	}

	/* step 4, step 5, step 6, step 7, step 8, output */
	reversed = state->reversed;
	for (k = 0, k2 = 0; k < n8; k++, k2 += 2) {
		cc_uint32 j = reversed[k], j4 = j << 2;
		e_1 = u[n2-j4-1]; e_2 = u[n2-j4-2];
		f_1 = u[j4+1];    f_2 = u[j4+0];

		g_1 =  e_1 + f_1 + C[k2+1] * (e_1 - f_1) + C[k2] * (e_2 + f_2);
		h_1 =  e_1 + f_1 - C[k2+1] * (e_1 - f_1) - C[k2] * (e_2 + f_2);
		g_2 =  e_2 - f_2 + C[k2+1] * (e_2 + f_2) - C[k2] * (e_1 - f_1);
		h_2 = -e_2 + f_2 + C[k2+1] * (e_2 + f_2) - C[k2] * (e_1 - f_1);

		x_1 = -0.5f * (g_1 * B[k2]   + g_2 * B[k2+1]);
		x_2 = -0.5f * (g_1 * B[k2+1] - g_2 * B[k2]);
		out[n4-1-k]   = -x_2;
		out[n4+k]     =  x_2;
		out[n3_4-1-k] =  x_1;
		out[n3_4+k]   =  x_1;

		y_1 = -0.5f * (h_1 * B[n2-2-k2] + h_2 * B[n2-1-k2]);
		y_2 = -0.5f * (h_1 * B[n2-1-k2] - h_2 * B[n2-2-k2]);
		out[k]      = -y_2;
		out[n2-1-k] =  y_2;
		out[n2+k]   =  y_1;
		out[n-1-k]  =  y_1;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Vorbis setup--------------------------------------------------------*
*#########################################################################################################################*/
struct Mode { cc_uint8 blockSizeFlag, mappingIdx; };
static cc_result Mode_DecodeSetup(struct VorbisState* ctx, struct Mode* m) {
	int windowType, transformType;
	m->blockSizeFlag = Vorbis_ReadBit(ctx);

	windowType    = Vorbis_ReadBits(ctx, 16);
	if (windowType != 0)    return VORBIS_ERR_MODE_WINDOW;
	transformType = Vorbis_ReadBits(ctx, 16);
	if (transformType != 0) return VORBIS_ERR_MODE_TRANSFORM;

	m->mappingIdx = Vorbis_ReadBits(ctx, 8);
	return 0;
}

static void Vorbis_CalcWindow(struct VorbisWindow* window, int blockSize) {
	int i, n = blockSize / 2;
	float *cur_window, *prev_window;
	double inner;

	window->Cur = window->Prev + n;
	cur_window  = window->Cur;
	prev_window = window->Prev;

	for (i = 0; i < n; i++) {
		inner          = Math_Sin((i + 0.5) / n * (PI/2));
		cur_window[i]  = Math_Sin((PI/2) * inner * inner);
	}
	for (i = 0; i < n; i++) {
		inner          = Math_Sin((i + 0.5) / n * (PI/2) + (PI/2));
		prev_window[i] = Math_Sin((PI/2) * inner * inner);
	}
}

void Vorbis_Free(struct VorbisState* ctx) {
	int i;
	for (i = 0; i < ctx->numCodebooks; i++) {
		Codebook_Free(&ctx->codebooks[i]);
	}

	Mem_Free(ctx->codebooks);
	Mem_Free(ctx->floors);
	Mem_Free(ctx->residues);
	Mem_Free(ctx->mappings);
	Mem_Free(ctx->modes);
	Mem_Free(ctx->windowRaw);
	Mem_Free(ctx->temp);
}

static cc_bool Vorbis_ValidBlockSize(cc_uint32 size) {
	return size >= 64 && size <= VORBIS_MAX_BLOCK_SIZE && Math_IsPowOf2(size);
}

static cc_result Vorbis_CheckHeader(struct VorbisState* ctx, cc_uint8 type) {
	cc_uint8 header[7];
	cc_bool OK;
	cc_result res;

	if ((res = Ogg_Read(ctx->source, header, sizeof(header)))) return res;
	if (header[0] != type) return VORBIS_ERR_WRONG_HEADER;

	OK = 
		header[1] == 'v' && header[2] == 'o' && header[3] == 'r' &&
		header[4] == 'b' && header[5] == 'i' && header[6] == 's';
	return OK ? 0 : ERR_INVALID_ARGUMENT;
}

static cc_result Vorbis_DecodeIdentifier(struct VorbisState* ctx) {
	cc_uint8 header[23];
	cc_uint32 version;
	cc_result res;

	if ((res = Ogg_Read(ctx->source, header, sizeof(header)))) return res;
	version  = Stream_GetU32_LE(&header[0]);
	if (version != 0) return VORBIS_ERR_VERSION;

	ctx->channels   = header[4];
	ctx->sampleRate = Stream_GetU32_LE(&header[5]);
	/* (12) bitrate_maximum, nominal, minimum */
	ctx->blockSizes[0] = 1 << (header[21] & 0xF);
	ctx->blockSizes[1] = 1 << (header[21] >>  4);

	if (!Vorbis_ValidBlockSize(ctx->blockSizes[0])) return VORBIS_ERR_BLOCKSIZE;
	if (!Vorbis_ValidBlockSize(ctx->blockSizes[1])) return VORBIS_ERR_BLOCKSIZE;
	if (ctx->blockSizes[0] > ctx->blockSizes[1])    return VORBIS_ERR_BLOCKSIZE;

	if (ctx->channels == 0 || ctx->channels > VORBIS_MAX_CHANS) return VORBIS_ERR_CHANS;
	/* check framing flag */
	return (header[22] & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static cc_result Vorbis_DecodeComments(struct VorbisState* ctx) {
	cc_uint32 i, len, comments;
	cc_uint8 flag;
	cc_result res;
	struct OggState* source = ctx->source;

	/* vendor name, followed by comments */
	if ((res = Ogg_ReadU32(source, &len)))      return res;
	if ((res = Ogg_Skip(source, len)))          return res;
	if ((res = Ogg_ReadU32(source, &comments))) return res;

	for (i = 0; i < comments; i++) {
		/* comments such as artist, year, etc */
		if ((res = Ogg_ReadU32(source, &len))) return res;
		if ((res = Ogg_Skip(source, len)))     return res;
	}

	/* check framing flag */
	if ((res = Ogg_ReadU8(source, &flag))) return res;
	return (flag & 1) ? 0 : VORBIS_ERR_FRAMING;
}

static cc_result Vorbis_DecodeSetup(struct VorbisState* ctx) {
	cc_uint32 framing, alignSkip;
	int i, count;
	cc_result res;

	count = Vorbis_ReadBits(ctx, 8) + 1;
	ctx->codebooks = (struct Codebook*)Mem_TryAlloc(count, sizeof(struct Codebook));
	if (!ctx->codebooks) return ERR_OUT_OF_MEMORY;

	for (i = 0; i < count; i++) {
		res = Codebook_DecodeSetup(ctx, &ctx->codebooks[i]);
		if (res) return res;
	}
	ctx->numCodebooks = count;

	count = Vorbis_ReadBits(ctx, 6) + 1;
	for (i = 0; i < count; i++) {
		int time = Vorbis_ReadBits(ctx, 16);
		if (time != 0) return VORBIS_ERR_TIME_TYPE;
	}

	count = Vorbis_ReadBits(ctx, 6) + 1;
	ctx->floors = (struct Floor*)Mem_TryAlloc(count, sizeof(struct Floor));
	if (!ctx->floors) return ERR_OUT_OF_MEMORY;

	for (i = 0; i < count; i++) {
		int floor = Vorbis_ReadBits(ctx, 16);
		if (floor != 1) return VORBIS_ERR_FLOOR_TYPE;

		res = Floor_DecodeSetup(ctx, &ctx->floors[i]);
		if (res) return res;
	}

	count = Vorbis_ReadBits(ctx, 6) + 1;
	ctx->residues = (struct Residue*)Mem_TryAlloc(count, sizeof(struct Residue));
	if (!ctx->residues) return ERR_OUT_OF_MEMORY;

	for (i = 0; i < count; i++) {
		int residue = Vorbis_ReadBits(ctx, 16);
		if (residue > 2) return VORBIS_ERR_FLOOR_TYPE;

		res = Residue_DecodeSetup(ctx, &ctx->residues[i], residue);
		if (res) return res;
	}

	count = Vorbis_ReadBits(ctx, 6) + 1;
	ctx->mappings = (struct Mapping*)Mem_TryAlloc(count, sizeof(struct Mapping));
	if (!ctx->mappings) return ERR_OUT_OF_MEMORY;

	for (i = 0; i < count; i++) {
		int mapping = Vorbis_ReadBits(ctx, 16);
		if (mapping != 0) return VORBIS_ERR_MAPPING_TYPE;

		res = Mapping_DecodeSetup(ctx, &ctx->mappings[i]);
		if (res) return res;
	}

	count = Vorbis_ReadBits(ctx, 6) + 1;
	ctx->modes = (struct Mode*)Mem_TryAlloc(count, sizeof(struct Mode));
	if (!ctx->modes) return ERR_OUT_OF_MEMORY;

	for (i = 0; i < count; i++) {
		res = Mode_DecodeSetup(ctx, &ctx->modes[i]);
		if (res) return res;
	}
	
	ctx->modeNumBits = iLog(count - 1); /* ilog([vorbis_mode_count]-1) bits */
	framing = Vorbis_ReadBit(ctx);
	Vorbis_AlignBits(ctx);
	/* check framing flag */
	return (framing & 1) ? 0 : VORBIS_ERR_FRAMING;
}

cc_result Vorbis_DecodeHeaders(struct VorbisState* ctx) {
	cc_uint32 count;
	cc_result res;
	
	if ((res = Vorbis_CheckHeader(ctx, 1)))   return res;
	if ((res = Vorbis_DecodeIdentifier(ctx))) return res;
	Ogg_DiscardPacket(ctx->source);

	if ((res = Vorbis_CheckHeader(ctx, 3)))   return res;
	if ((res = Vorbis_DecodeComments(ctx)))   return res;
	Ogg_DiscardPacket(ctx->source);

	if ((res = Vorbis_CheckHeader(ctx, 5)))   return res;
	if ((res = Vorbis_DecodeSetup(ctx)))      return res;
	Ogg_DiscardPacket(ctx->source);

	/* window calculations can be pre-computed here */
	count = ctx->blockSizes[0] + ctx->blockSizes[1];
	ctx->windowRaw = (float*)Mem_TryAlloc(count, sizeof(float));
	if (!ctx->windowRaw) return ERR_OUT_OF_MEMORY;

	ctx->windows[0].Prev = ctx->windowRaw;
	ctx->windows[1].Prev = ctx->windowRaw + ctx->blockSizes[0];

	Vorbis_CalcWindow(&ctx->windows[0], ctx->blockSizes[0]);
	Vorbis_CalcWindow(&ctx->windows[1], ctx->blockSizes[1]);

	count     = ctx->channels * ctx->blockSizes[1];
	ctx->temp = (float*)Mem_TryAllocCleared(count * 3, sizeof(float));
	if (!ctx->temp) return ERR_OUT_OF_MEMORY;

	ctx->values[0] = ctx->temp + count;
	ctx->values[1] = ctx->temp + count * 2;

	imdct_init(&ctx->imdct[0], ctx->blockSizes[0]);
	imdct_init(&ctx->imdct[1], ctx->blockSizes[1]);
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Vorbis frame--------------------------------------------------------*
*#########################################################################################################################*/
cc_result Vorbis_DecodeFrame(struct VorbisState* ctx) {
	/* frame header */
	cc_uint32 packetType;
	struct Mapping* mapping;
	struct Mode* mode;
	int modeIdx;

	/* floor/residue */
	cc_bool hasFloor[VORBIS_MAX_CHANS];
	cc_bool hasResidue[VORBIS_MAX_CHANS];
	cc_bool doNotDecode[VORBIS_MAX_CHANS];
	float* data[VORBIS_MAX_CHANS];
	int submap, floorIdx;
	int ch, residueIdx;

	/* inverse coupling */
	int magChannel, angChannel;
	float* magValues, m;
	float* angValues, a;

	/* misc variables */
	float* tmp;
	cc_uint32 alignSkip;
	int i, j; 
	cc_result res;
	
	res = Vorbis_TryReadBits(ctx, 1, &packetType);
	if (res) return res;
	if (packetType) return VORBIS_ERR_FRAME_TYPE;

	modeIdx = Vorbis_ReadBits(ctx, ctx->modeNumBits);
	mode    = &ctx->modes[modeIdx];
	mapping = &ctx->mappings[mode->mappingIdx];

	/* decode window shape */
	ctx->curBlockSize = ctx->blockSizes[mode->blockSizeFlag];
	ctx->dataSize     = ctx->curBlockSize / 2;
	/* long window lapping flags - we don't care about them though */
	if (mode->blockSizeFlag) { Vorbis_ReadBits(ctx, 2); } /* TODO: do we just SkipBits here */

	/* swap prev and cur outputs around */
	tmp = ctx->values[1]; ctx->values[1] = ctx->values[0]; ctx->values[0] = tmp;
	Mem_Set(ctx->values[0], 0, ctx->channels * ctx->curBlockSize);

	for (i = 0; i < ctx->channels; i++) {
		ctx->curOutput[i]  = ctx->values[0] + i * ctx->curBlockSize;
		ctx->prevOutput[i] = ctx->values[1] + i * ctx->prevBlockSize;
	}

	/* decode floor */
	for (i = 0; i < ctx->channels; i++) {
		submap   = mapping->mux[i];
		floorIdx = mapping->floorIdx[submap];
		hasFloor[i]   = Floor_DecodeFrame(ctx, &ctx->floors[floorIdx], i);
		hasResidue[i] = hasFloor[i];
	}

	/* non-zero vector propogate */
	for (i = 0; i < mapping->couplingSteps; i++) {
		magChannel = mapping->magnitude[i];
		angChannel = mapping->angle[i];

		if (hasResidue[magChannel] || hasResidue[angChannel]) {
			hasResidue[magChannel] = true; hasResidue[angChannel] = true;
		}
	}

	/* decode residue */
	for (i = 0; i < mapping->submaps; i++) {
		ch = 0;
		/* map residue data to actual channel data */
		for (j = 0; j < ctx->channels; j++) {
			if (mapping->mux[j] != i) continue;

			doNotDecode[ch] = !hasResidue[j];
			data[ch] = ctx->curOutput[j];
			ch++;
		}

		residueIdx = mapping->floorIdx[i];
		Residue_DecodeFrame(ctx, &ctx->residues[residueIdx], ch, doNotDecode, data);
	}

	/* inverse coupling */
	for (i = mapping->couplingSteps - 1; i >= 0; i--) {
		magValues = ctx->curOutput[mapping->magnitude[i]];
		angValues = ctx->curOutput[mapping->angle[i]];

		for (j = 0; j < ctx->dataSize; j++) {
			m = magValues[j]; a = angValues[j];

			if (m > 0.0f) {
				if (a > 0.0f) { angValues[j] = m - a; }
				else {
					angValues[j] = m;
					magValues[j] = m + a;
				}
			} else {
				if (a > 0.0f) { angValues[j] = m + a; }
				else {
					angValues[j] = m;
					magValues[j] = m - a;
				}
			}
		}
	}

	/* compute dot product of floor and residue, producing audio spectrum vector */
	for (i = 0; i < ctx->channels; i++) {
		if (!hasFloor[i]) continue;

		submap   = mapping->mux[i];
		floorIdx = mapping->floorIdx[submap];
		Floor_Synthesis(ctx, &ctx->floors[floorIdx], i);
	}

	/* inverse monolithic transform of audio spectrum vector */
	for (i = 0; i < ctx->channels; i++) {
		tmp = ctx->curOutput[i];

		if (!hasFloor[i]) {
			/* TODO: Do we actually need to zero data here (residue type 2 maybe) */
			Mem_Set(tmp, 0, ctx->curBlockSize * sizeof(float));
		} else {
			imdct_calc(tmp, tmp, &ctx->imdct[mode->blockSizeFlag]);
			/* defer windowing until output */
		}
	}

	/* discard remaining bits at end of packet */
	Vorbis_AlignBits(ctx);
	return 0;
}

int Vorbis_OutputFrame(struct VorbisState* ctx, cc_int16* data) {
	struct VorbisWindow window;
	float* prev[VORBIS_MAX_CHANS];
	float*  cur[VORBIS_MAX_CHANS];

	int curQrtr, prevQrtr, overlapQtr;
	int curOffset, prevOffset, overlapSize;
	float sample;
	int i, ch;

	/* first frame decoded has no data */
	if (ctx->prevBlockSize == 0) {
		ctx->prevBlockSize = ctx->curBlockSize;
		return 0;
	}

	/* data returned is from centre of previous block to centre of current block */
	/* data is aligned, such that 3/4 of prev block is aligned to 1/4 of cur block */
	curQrtr   = ctx->curBlockSize  / 4;
	prevQrtr  = ctx->prevBlockSize / 4;
	overlapQtr = min(curQrtr, prevQrtr);
	
	/* So for example, consider a short block overlapping with a long block
	   a) we need to chop off 'prev' before its halfway point
	   b) then need to chop off the 'cur' before the halfway point of 'prev'
	             |-   ********|*****                        |-   ********|
	            -| - *        |     ***                     | - *        |
	           - |  #         |        ***          ===>    |  #         |
	          -  | * -        |           ***               | * -        |
	   ******-***|*   -       |              ***            |*   -       |
	*/	
	curOffset  = curQrtr  - overlapQtr;
	prevOffset = prevQrtr - overlapQtr;

	for (i = 0; i < ctx->channels; i++) {
		prev[i] = ctx->prevOutput[i] + (prevQrtr * 2);
		cur[i]  = ctx->curOutput[i];
	}

	/* for long prev and short cur block, there will be non-overlapped data before */
	for (i = 0; i < prevOffset; i++) {
		for (ch = 0; ch < ctx->channels; ch++) {
			sample = prev[ch][i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (cc_int16)(sample * 32767);
		}
	}

	/* adjust pointers to start at 0 for overlapping */
	for (i = 0; i < ctx->channels; i++) {
		prev[i] += prevOffset; cur[i] += curOffset;
	}

	overlapSize = overlapQtr * 2;
	window = ctx->windows[(overlapQtr * 4) == ctx->blockSizes[1]];

	/* overlap and add data */
	/* also perform windowing here */
	for (i = 0; i < overlapSize; i++) {
		for (ch = 0; ch < ctx->channels; ch++) {
			sample = prev[ch][i] * window.Prev[i] + cur[ch][i] * window.Cur[i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (cc_int16)(sample * 32767);
		}
	}

	/* for long cur and short prev block, there will be non-overlapped data after */
	for (i = 0; i < ctx->channels; i++) { cur[i] += overlapSize; }
	for (i = 0; i < curOffset; i++) {
		for (ch = 0; ch < ctx->channels; ch++) {
			sample = cur[ch][i];
			Math_Clamp(sample, -1.0f, 1.0f);
			*data++ = (cc_int16)(sample * 32767);
		}
	}

	ctx->prevBlockSize = ctx->curBlockSize;
	return (prevQrtr + curQrtr) * ctx->channels;
}
