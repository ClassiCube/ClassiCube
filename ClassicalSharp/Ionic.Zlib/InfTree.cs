// Originally copyright (c) 2009 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// See license.txt, section Ionic.Zlib license
#if __MonoCS__
using System;
namespace Ionic.Zlib {
	
	sealed class InfTree {
		
		const int MANY = 1440;
		
		static readonly int[] fixed_tl = {96, 7, 256, 0, 8, 80, 0, 8, 16, 84, 8, 115, 82, 7, 31, 0, 8, 112, 0, 8, 48, 0, 9, 192, 80, 7, 10, 0, 8, 96, 0, 8, 32, 0, 9, 160, 0, 8, 0, 0, 8, 128, 0, 8, 64, 0, 9, 224, 80, 7, 6, 0, 8, 88, 0, 8, 24, 0, 9, 144, 83, 7, 59, 0, 8, 120, 0, 8, 56, 0, 9, 208, 81, 7, 17, 0, 8, 104, 0, 8, 40, 0, 9, 176, 0, 8, 8, 0, 8, 136, 0, 8, 72, 0, 9, 240, 80, 7, 4, 0, 8, 84, 0, 8, 20, 85, 8, 227, 83, 7, 43, 0, 8, 116, 0, 8, 52, 0, 9, 200, 81, 7, 13, 0, 8, 100, 0, 8, 36, 0, 9, 168, 0, 8, 4, 0, 8, 132, 0, 8, 68, 0, 9, 232, 80, 7, 8, 0, 8, 92, 0, 8, 28, 0, 9, 152, 84, 7, 83, 0, 8, 124, 0, 8, 60, 0, 9, 216, 82, 7, 23, 0, 8, 108, 0, 8, 44, 0, 9, 184, 0, 8, 12, 0, 8, 140, 0, 8, 76, 0, 9, 248, 80, 7, 3, 0, 8, 82, 0, 8, 18, 85, 8, 163, 83, 7, 35, 0, 8, 114, 0, 8, 50, 0, 9, 196, 81, 7, 11, 0, 8, 98, 0, 8, 34, 0, 9, 164, 0, 8, 2, 0, 8, 130, 0, 8, 66, 0, 9, 228, 80, 7, 7, 0, 8, 90, 0, 8, 26, 0, 9, 148, 84, 7, 67, 0, 8, 122, 0, 8, 58, 0, 9, 212, 82, 7, 19, 0, 8, 106, 0, 8, 42, 0, 9, 180, 0, 8, 10, 0, 8, 138, 0, 8, 74, 0, 9, 244, 80, 7, 5, 0, 8, 86, 0, 8, 22, 192, 8, 0, 83, 7, 51, 0, 8, 118, 0, 8, 54, 0, 9, 204, 81, 7, 15, 0, 8, 102, 0, 8, 38, 0, 9, 172, 0, 8, 6, 0, 8, 134, 0, 8, 70, 0, 9, 236, 80, 7, 9, 0, 8, 94, 0, 8, 30, 0, 9, 156, 84, 7, 99, 0, 8, 126, 0, 8, 62, 0, 9, 220, 82, 7, 27, 0, 8, 110, 0, 8, 46, 0, 9, 188, 0, 8, 14, 0, 8, 142, 0, 8, 78, 0, 9, 252, 96, 7, 256, 0, 8, 81, 0, 8, 17, 85, 8, 131, 82, 7, 31, 0, 8, 113, 0, 8, 49, 0, 9, 194, 80, 7, 10, 0, 8, 97, 0, 8, 33, 0, 9, 162, 0, 8, 1, 0, 8, 129, 0, 8, 65, 0, 9, 226, 80, 7, 6, 0, 8, 89, 0, 8, 25, 0, 9, 146, 83, 7, 59, 0, 8, 121, 0, 8, 57, 0, 9, 210, 81, 7, 17, 0, 8, 105, 0, 8, 41, 0, 9, 178, 0, 8, 9, 0, 8, 137, 0, 8, 73, 0, 9, 242, 80, 7, 4, 0, 8, 85, 0, 8, 21, 80, 8, 258, 83, 7, 43, 0, 8, 117, 0, 8, 53, 0, 9, 202, 81, 7, 13, 0, 8, 101, 0, 8, 37, 0, 9, 170, 0, 8, 5, 0, 8, 133, 0, 8, 69, 0, 9, 234, 80, 7, 8, 0, 8, 93, 0, 8, 29, 0, 9, 154, 84, 7, 83, 0, 8, 125, 0, 8, 61, 0, 9, 218, 82, 7, 23, 0, 8, 109, 0, 8, 45, 0, 9, 186,
			0, 8, 13, 0, 8, 141, 0, 8, 77, 0, 9, 250, 80, 7, 3, 0, 8, 83, 0, 8, 19, 85, 8, 195, 83, 7, 35, 0, 8, 115, 0, 8, 51, 0, 9, 198, 81, 7, 11, 0, 8, 99, 0, 8, 35, 0, 9, 166, 0, 8, 3, 0, 8, 131, 0, 8, 67, 0, 9, 230, 80, 7, 7, 0, 8, 91, 0, 8, 27, 0, 9, 150, 84, 7, 67, 0, 8, 123, 0, 8, 59, 0, 9, 214, 82, 7, 19, 0, 8, 107, 0, 8, 43, 0, 9, 182, 0, 8, 11, 0, 8, 139, 0, 8, 75, 0, 9, 246, 80, 7, 5, 0, 8, 87, 0, 8, 23, 192, 8, 0, 83, 7, 51, 0, 8, 119, 0, 8, 55, 0, 9, 206, 81, 7, 15, 0, 8, 103, 0, 8, 39, 0, 9, 174, 0, 8, 7, 0, 8, 135, 0, 8, 71, 0, 9, 238, 80, 7, 9, 0, 8, 95, 0, 8, 31, 0, 9, 158, 84, 7, 99, 0, 8, 127, 0, 8, 63, 0, 9, 222, 82, 7, 27, 0, 8, 111, 0, 8, 47, 0, 9, 190, 0, 8, 15, 0, 8, 143, 0, 8, 79, 0, 9, 254, 96, 7, 256, 0, 8, 80, 0, 8, 16, 84, 8, 115, 82, 7, 31, 0, 8, 112, 0, 8, 48, 0, 9, 193, 80, 7, 10, 0, 8, 96, 0, 8, 32, 0, 9, 161, 0, 8, 0, 0, 8, 128, 0, 8, 64, 0, 9, 225, 80, 7, 6, 0, 8, 88, 0, 8, 24, 0, 9, 145, 83, 7, 59, 0, 8, 120, 0, 8, 56, 0, 9, 209, 81, 7, 17, 0, 8, 104, 0, 8, 40, 0, 9, 177, 0, 8, 8, 0, 8, 136, 0, 8, 72, 0, 9, 241, 80, 7, 4, 0, 8, 84, 0, 8, 20, 85, 8, 227, 83, 7, 43, 0, 8, 116, 0, 8, 52, 0, 9, 201, 81, 7, 13, 0, 8, 100, 0, 8, 36, 0, 9, 169, 0, 8, 4, 0, 8, 132, 0, 8, 68, 0, 9, 233, 80, 7, 8, 0, 8, 92, 0, 8, 28, 0, 9, 153, 84, 7, 83, 0, 8, 124, 0, 8, 60, 0, 9, 217, 82, 7, 23, 0, 8, 108, 0, 8, 44, 0, 9, 185, 0, 8, 12, 0, 8, 140, 0, 8, 76, 0, 9, 249, 80, 7, 3, 0, 8, 82, 0, 8, 18, 85, 8, 163, 83, 7, 35, 0, 8, 114, 0, 8, 50, 0, 9, 197, 81, 7, 11, 0, 8, 98, 0, 8, 34, 0, 9, 165, 0, 8, 2, 0, 8, 130, 0, 8, 66, 0, 9, 229, 80, 7, 7, 0, 8, 90, 0, 8, 26, 0, 9, 149, 84, 7, 67, 0, 8, 122, 0, 8, 58, 0, 9, 213, 82, 7, 19, 0, 8, 106, 0, 8, 42, 0, 9, 181, 0, 8, 10, 0, 8, 138, 0, 8, 74, 0, 9, 245, 80, 7, 5, 0, 8, 86, 0, 8, 22, 192, 8, 0, 83, 7, 51, 0, 8, 118, 0, 8, 54, 0, 9, 205, 81, 7, 15, 0, 8, 102, 0, 8, 38, 0, 9, 173, 0, 8, 6, 0, 8, 134, 0, 8, 70, 0, 9, 237, 80, 7, 9, 0, 8, 94, 0, 8, 30, 0, 9, 157, 84, 7, 99, 0, 8, 126, 0, 8, 62, 0, 9, 221, 82, 7, 27, 0, 8, 110, 0, 8, 46, 0, 9, 189, 0, 8,
			14, 0, 8, 142, 0, 8, 78, 0, 9, 253, 96, 7, 256, 0, 8, 81, 0, 8, 17, 85, 8, 131, 82, 7, 31, 0, 8, 113, 0, 8, 49, 0, 9, 195, 80, 7, 10, 0, 8, 97, 0, 8, 33, 0, 9, 163, 0, 8, 1, 0, 8, 129, 0, 8, 65, 0, 9, 227, 80, 7, 6, 0, 8, 89, 0, 8, 25, 0, 9, 147, 83, 7, 59, 0, 8, 121, 0, 8, 57, 0, 9, 211, 81, 7, 17, 0, 8, 105, 0, 8, 41, 0, 9, 179, 0, 8, 9, 0, 8, 137, 0, 8, 73, 0, 9, 243, 80, 7, 4, 0, 8, 85, 0, 8, 21, 80, 8, 258, 83, 7, 43, 0, 8, 117, 0, 8, 53, 0, 9, 203, 81, 7, 13, 0, 8, 101, 0, 8, 37, 0, 9, 171, 0, 8, 5, 0, 8, 133, 0, 8, 69, 0, 9, 235, 80, 7, 8, 0, 8, 93, 0, 8, 29, 0, 9, 155, 84, 7, 83, 0, 8, 125, 0, 8, 61, 0, 9, 219, 82, 7, 23, 0, 8, 109, 0, 8, 45, 0, 9, 187, 0, 8, 13, 0, 8, 141, 0, 8, 77, 0, 9, 251, 80, 7, 3, 0, 8, 83, 0, 8, 19, 85, 8, 195, 83, 7, 35, 0, 8, 115, 0, 8, 51, 0, 9, 199, 81, 7, 11, 0, 8, 99, 0, 8, 35, 0, 9, 167, 0, 8, 3, 0, 8, 131, 0, 8, 67, 0, 9, 231, 80, 7, 7, 0, 8, 91, 0, 8, 27, 0, 9, 151, 84, 7, 67, 0, 8, 123, 0, 8, 59, 0, 9, 215, 82, 7, 19, 0, 8, 107, 0, 8, 43, 0, 9, 183, 0, 8, 11, 0, 8, 139, 0, 8, 75, 0, 9, 247, 80, 7, 5, 0, 8, 87, 0, 8, 23, 192, 8, 0, 83, 7, 51, 0, 8, 119, 0, 8, 55, 0, 9, 207, 81, 7, 15, 0, 8, 103, 0, 8, 39, 0, 9, 175, 0, 8, 7, 0, 8, 135, 0, 8, 71, 0, 9, 239, 80, 7, 9, 0, 8, 95, 0, 8, 31, 0, 9, 159, 84, 7, 99, 0, 8, 127, 0, 8, 63, 0, 9, 223, 82, 7, 27, 0, 8, 111, 0, 8, 47, 0, 9, 191, 0, 8, 15, 0, 8, 143, 0, 8, 79, 0, 9, 255};

		static readonly int[] fixed_td = {80, 5, 1, 87, 5, 257, 83, 5, 17, 91, 5, 4097, 81, 5, 5, 89, 5, 1025, 85, 5, 65, 93, 5, 16385, 80, 5, 3, 88, 5, 513, 84, 5, 33, 92, 5, 8193, 82, 5, 9, 90, 5, 2049, 86, 5, 129, 192, 5, 24577, 80, 5, 2, 87, 5, 385, 83, 5, 25, 91, 5, 6145, 81, 5, 7, 89, 5, 1537, 85, 5, 97, 93, 5, 24577, 80, 5, 4, 88, 5, 769, 84, 5, 49, 92, 5, 12289, 82, 5, 13, 90, 5, 3073, 86, 5, 193, 192, 5, 24577};
		
		// Tables for deflate from PKZIP's appnote.txt.
		static readonly int[] cplens = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
		
		// see note #13 above about 258
		static readonly int[] cplext = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112};
		
		static readonly int[] cpdist = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
		
		static readonly int[] cpdext = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
		
		// If BMAX needs to be larger than 16, then h and x[] should be uLong.
		const int BMAX = 15; // maximum bit length of any code
		
		int hn = 0; // hufts used in space
		int[] v = null; // work area for huft_build
		int[] c = new int[BMAX + 1]; // bit length count table
		int r0 = 0, r1 = 0, r2 = 0; // table entry for structure assignment
		int[] tableStack = new int[BMAX]; // table stack
		int[] x = new int[BMAX + 1]; // bit offsets, then code stack
		
		int BuildTree(int[] b, int bindex, int n, int s, int[] d, int[] e, ref int t, ref int m, int[] hp) {
			// Given a list of code lengths and a maximum table size, make a set of
			// tables to decode that set of codes.  Return Z_OK on success, Z_BUF_ERROR
			// if the given code set is incomplete (the tables are still built in this
			// case), Z_DATA_ERROR if the input is invalid (an over-subscribed set of
			// lengths), or Z_MEM_ERROR if not enough memory.
			
			int f; // i repeats in table every f entries
			int i; // counter, current code
			int j; // counter
			int p = 0; // pointer into c[], b[], or v[]
			int y; // number of dummy codes added
			
			
			// Generate counts for each bit length
			i = n;
			do {
				c[b[bindex + p]]++;
				p++;
				i--; // assume all entries <= BMAX
			} while (i != 0);
			
			if (c[0] == n) {
				// null input--all zero length codes
				t = -1;
				m = 0;
				return RCode.Okay;
			}
			
			// Find minimum and maximum length, bound *m by those
			int bitsPerTable = m; // bits per table (returned in m)
			for (j = 1; j <= BMAX; j++)
				if (c[j] != 0)
					break;
			
			int curCodeBits = j; // number of bits in current code (starting at minimum code length)
			if (bitsPerTable < curCodeBits) {
				bitsPerTable = curCodeBits;
			}
			
			for (i = BMAX; i != 0; i--) {
				if (c[i] != 0)
					break;
			}
			
			int maxCodeLen = i; // maximum code length
			if (bitsPerTable > maxCodeLen) {
				bitsPerTable = maxCodeLen;
			}
			m = bitsPerTable;
			
			// Adjust last length count to fill out codes, if needed
			for (y = 1 << j; j < i; j++, y <<= 1) {
				if ((y -= c[j]) < 0) {
					return RCode.DataError;
				}
			}
			if ((y -= c[i]) < 0) {
				return RCode.DataError;
			}
			c[i] += y;
			
			// Generate starting offsets into the value table for each length
			x[1] = j = 0;
			p = 1;
			int xIndex = 2; // pointer into x
			while (--i != 0) {
				// note that i == g from above
				x[xIndex++] = (j += c[p]);
				p++;
			}
			
			// Make a table of values in order of bit lengths
			i = 0;
			p = 0;
			do {
				if ((j = b[bindex + p]) != 0) {
					v[x[j]++] = i;
				}
				p++;
			} while (++i < n);
			n = x[maxCodeLen]; // set n to length of v
			
			// Generate the Huffman codes and for each, make the table entries
			x[0] = i = 0; // first Huffman code is zero
			p = 0; // grab values in bit order
			int h = -1; // table level, no tables yet so level is -1
			int w = -bitsPerTable; // bits before this table == (l * h), bits decoded == (l * h)
			tableStack[0] = 0; // just to keep compilers happy
			int q = 0; // points to current table
			int z = 0; // number of entries in current table
			
			// go through the bit lengths (k already is bits in shortest code)
			for (; curCodeBits <= maxCodeLen; curCodeBits++)
			{
				int a = c[curCodeBits]; // counter for codes of length k
				while (a-- != 0)
				{
					// here i is the Huffman code of length k bits for value *p
					// make tables up to required level
					while (curCodeBits > w + bitsPerTable)
					{
						h++;
						w += bitsPerTable; // previous table always l bits
						// compute minimum size table less than or equal to l bits
						z = maxCodeLen - w;
						z = (z > bitsPerTable)?bitsPerTable:z; // table size upper limit
						if ((f = 1 << (j = curCodeBits - w)) > a + 1)
						{
							// try a k-w bit table
							// too few codes for k-w bit table
							f -= (a + 1); // deduct codes from patterns left
							xIndex = curCodeBits;
							if (j < z)
							{
								while (++j < z)
								{
									// try smaller tables up to z bits
									if ((f <<= 1) <= c[++xIndex])
										break; // enough codes to use up j bits
									f -= c[xIndex]; // else deduct codes from patterns
								}
							}
						}
						z = 1 << j; // table entries for j-bit table
						
						// allocate new table
						if (hn + z > MANY) {
							return RCode.DataError; // overflow of MANY
						}
						tableStack[h] = q = hn; // DEBUG
						hn += z;
						
						// connect to last table, if there is one
						if (h != 0) {
							x[h] = i; // save pattern for backing up
							r0 = (sbyte) j; // bits in this table
							r1 = (sbyte) bitsPerTable; // bits to dump before this table
							j = URShift(i, (w - bitsPerTable));
							r2 = (q - tableStack[h - 1] - j); // offset to this table
							
							int dstIndex = (tableStack[h - 1] + j) * 3; // connect to last table
							hp[dstIndex++] = r0;
							hp[dstIndex++] = r1;
							hp[dstIndex++] = r2;
						} else {
							t = q; // first table is returned result
						}
					}
					
					// set up table entry in r
					r1 = (sbyte) (curCodeBits - w);
					if (p >= n) {
						r0 = 128 + 64; // out of values--invalid code
					} else if (v[p] < s) {
						r0 = (sbyte) (v[p] < 256 ? 0: 32 + 64); // 256 is end-of-block
						r2 = v[p++]; // simple code is just the value
					} else {
						r0 = (sbyte) (e[v[p] - s] + 16 + 64); // non-simple--look up in lists
						r2 = d[v[p++] - s];
					}
					
					// fill code-like entries with r
					f = 1 << (curCodeBits - w);
					for (j = URShift(i, w); j < z; j += f) {
						int dstIndex = (q + j) * 3;
						hp[dstIndex++] = r0;
						hp[dstIndex++] = r1;
						hp[dstIndex++] = r2;
					}
					
					// backwards increment the k-bit code i
					for (j = 1 << (curCodeBits - 1); (i & j) != 0; j = URShift(j, 1)) {
						i ^= j;
					}
					i ^= j;
					
					// backup over finished tables
					int mask = (1 << w) - 1;
					while ((i & mask) != x[h]) {
						h--; // don't need to update q
						w -= bitsPerTable;
						mask = (1 << w) - 1;
					}
				}
			}
			// Return Z_BUF_ERROR if we were given an incomplete table
			return y != 0 && maxCodeLen != 1 ? RCode.BufferError : RCode.Okay;
		}
		
		internal void InflateTreeBits(int[] c, ref int bb, ref int tb, int[] hp, ZlibCodec z) {
			ResetWorkArea( 19 );
			hn = 0;
			int result = BuildTree(c, 0, 19, 19, null, null, ref tb, ref bb, hp);
			
			if (result == RCode.DataError) {
				throw new InvalidOperationException( "oversubscribed dynamic bit lengths tree" );
			} else if (result == RCode.BufferError || bb == 0) {
				throw new InvalidOperationException( "incomplete dynamic bit lengths tree" );
			}
		}
		
		internal void InflateTreesDynamic(int nl, int nd, int[] c, ref int bl, ref int bd, 
		                                  ref int tl, ref int td, int[] hp) {
			// build literal/length tree
			ResetWorkArea( 288 );
			hn = 0;
			int result = BuildTree(c, 0, nl, 257, cplens, cplext, ref tl, ref bl, hp);
			if (result != RCode.Okay || bl == 0) {
				string message = null;
				if (result == RCode.DataError) {
					message = "oversubscribed literal/length tree";
				} else {
					message = "incomplete literal/length tree";
				}
				throw new InvalidOperationException( "Unable to inflate dynamic tree: " + message );
			}
			
			// build distance tree
			ResetWorkArea( 288 );
			result = BuildTree(c, nl, nd, 0, cpdist, cpdext, ref td, ref bd, hp);
			
			if (result != RCode.Okay || (bd == 0 && nl > 257)) {
				string message = null;
				if (result == RCode.DataError) {
					message = "oversubscribed distance tree";
				} else if (result == RCode.BufferError) {
					message = "incomplete distance tree";
				} else {
					message = "empty distance tree with lengths";
				}
				throw new InvalidOperationException( "Unable to inflate dynamic tree: " + message );
			}
		}
		
		internal static void InflateTreesFixed( out int bl, out int bd, out int[] tl, out int[] td ) {
			bl = 9;
			bd = 5;
			tl = fixed_tl;
			td = fixed_td;
		}
		
		void ResetWorkArea( int vsize ) {
			if( v == null || v.Length < vsize ) {
				v = new int[vsize];
			}
			Array.Clear(v, 0, vsize);
			Array.Clear(c, 0, BMAX + 1);
			r0 = 0; r1 = 0; r2 = 0;
			Array.Clear(tableStack, 0, BMAX);
			Array.Clear(x, 0, BMAX + 1);
		}
		
		static int URShift( int value, int bits ) {
			return (int)( (uint)value >> bits );
		}
	}
}
#endif