// Originally copyright (c) 2009 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// See license.txt, section Ionic.Zlib license
#if __MonoCS__
using System;
using System.IO;

namespace Ionic.Zlib
{
	sealed class InflateBlocks
	{
		const int MANY = 1440;

		// Table for deflate from PKZIP's appnote.txt.
		static readonly int[] border = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

		enum InflateBlockMode {
			TYPE   = 0,                     // get type bits (3, including end bit)
			LENS   = 1,                     // get lengths for stored
			STORED = 2,                     // processing stored block
			TABLE  = 3,                     // get table lengths
			BTREE  = 4,                     // get bit lengths tree for a dynamic block
			DTREE  = 5,                     // get length, distance trees for a dynamic block
			CODES  = 6,                     // processing fixed or dynamic block
			DRY    = 7,                     // output remaining window bytes
			DONE   = 8,                     // finished last block, done
		}

		InflateBlockMode mode;                    // current inflate_block mode

		int left;                                // if STORED, bytes left to copy

		int table;                               // table lengths (14 bits)
		int index;                               // index into blens (or border)
		int[] blens;                             // bit lengths of codes
		int bb;                   // bit length tree depth
		int tb;                   // bit length decoding tree

		InflateCodes codes = new InflateCodes(); // if CODES, current state
		int last;                                // true if this block is the last block
		internal ZlibCodec codec;                        // pointer back to this zlib stream

		// mode independent information
		internal int bitk;                                // bits in bit buffer
		internal int bitb;                                // bit buffer
		internal int[] hufts;                             // single malloc for tree space
		internal byte[] window;                           // sliding window
		internal int end;                                 // one byte after sliding window
		internal int readAt;                              // window read pointer
		internal int writeAt;                             // window write pointer

		InfTree inftree = new InfTree();

		internal InflateBlocks(ZlibCodec codec, int w) {
			this.codec = codec;
			hufts = new int[MANY * 3];
			window = new byte[w];
			end = w;
			mode = InflateBlockMode.TYPE;
			Reset();
		}

		internal void Reset() {
			mode = InflateBlockMode.TYPE;
			bitk = 0;
			bitb = 0;
			readAt = writeAt = 0;
		}

		internal int Process(int r) {
			int t; // temporary storage
			int nextIn = codec.NextIn; // input data pointer
			int availIn = codec.AvailableBytesIn; // bytes available there
			int bits = bitb; // bit buffer
			int bitsNum = bitk; // bits in bit buffer
			int q = writeAt; // output window write pointer
			int m = q < readAt ? readAt - q - 1 : end - q; // bytes to end of window or read pointer

			// process input based on current state
			while (true)
			{
				switch (mode)
				{
					case InflateBlockMode.TYPE:
						while( bitsNum < 3 ) {
							if (availIn != 0) {
								r = RCode.Okay;
							} else {
								return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}

							availIn--;
							bits |= codec.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}
						
						last = bits & 0x1;
						switch( ( bits & 0x7 ) >> 1 ) {
							case 0:  // stored
								bits >>= 3; bitsNum -= 3;
								t = bitsNum & 7; // go to byte boundary
								bits >>= t; bitsNum -= t;
								mode = InflateBlockMode.LENS; // get length of stored block
								break;

							case 1:  // fixed
								int bl, bd;
								int[] tl, td;
								InfTree.InflateTreesFixed(out bl, out bd, out tl, out td);
								codes.Init(bl, bd, tl, 0, td, 0);
								bits >>= 3; bitsNum -= 3;
								mode = InflateBlockMode.CODES;
								break;

							case 2:  // dynamic
								bits >>= 3; bitsNum -= 3;
								mode = InflateBlockMode.TABLE;
								break;

							case 3:  // illegal
								throw new InvalidDataException( "invalid block type" );
						} break;

					case InflateBlockMode.LENS:
						while( bitsNum < 32 ) {
							if( availIn != 0 ) {
								r = RCode.Okay;
							} else {
								return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}
							availIn--;
							bits |= codec.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						if ( ( ( ~bits >> 16 ) & 0xffff ) != ( bits & 0xffff ) ) {
							throw new InvalidDataException( "invalid stored block lengths" );
						}
						left = bits & 0xffff;
						bits = bitsNum = 0; // dump bits
						mode = left != 0 ? InflateBlockMode.STORED : (last != 0 ? InflateBlockMode.DRY : InflateBlockMode.TYPE);
						break;

					case InflateBlockMode.STORED:
						if( availIn == 0 ) {
							return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
						}

						if( m == 0 ) {
							if( q == end && readAt != 0 ) {
								q = 0;
								m = q < readAt ? readAt - q - 1 : end - q;
							}
							if( m == 0 ) {
								writeAt = q;
								r = Flush(r);
								q = writeAt;
								m = q < readAt ? readAt - q - 1 : end - q;
								
								if( q == end && readAt != 0 ) {
									q = 0;
									m = q < readAt ? readAt - q - 1 : end - q;
								}
								if( m == 0 ) {
									return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
								}
							}
						}
						r = RCode.Okay;

						t = left;
						if (t > availIn)
							t = availIn;
						if (t > m)
							t = m;
						Array.Copy(codec.InputBuffer, nextIn, window, q, t);
						nextIn += t; availIn -= t;
						q += t; m -= t;
						if ((left -= t) != 0)
							break;
						mode = last != 0 ? InflateBlockMode.DRY : InflateBlockMode.TYPE;
						break;

					case InflateBlockMode.TABLE:
						while (bitsNum < 14) {
							if( availIn != 0 ) {
								r = RCode.Okay;
							} else {
								return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}

							availIn--;
							bits |= codec.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						table = t = (bits & 0x3fff);
						if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29) {
							throw new InvalidDataException( "too many length or distance symbols" );
						}
						
						t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
						if (blens == null || blens.Length < t) {
							blens = new int[t];
						} else {
							Array.Clear(blens, 0, t);
						}

						bits >>= 14;
						bitsNum -= 14;

						index = 0;
						mode = InflateBlockMode.BTREE;
						goto case InflateBlockMode.BTREE;

					case InflateBlockMode.BTREE:
						while (index < 4 + (table >> 10)) {
							while( bitsNum < 3 ) {
								if( availIn != 0 ) {
									r = RCode.Okay;
								} else {
									return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
								}

								availIn--;
								bits |= codec.InputBuffer[nextIn++] << bitsNum;
								bitsNum += 8;
							}

							blens[border[index++]] = bits & 7;
							bits >>= 3; bitsNum -= 3;
						}

						while (index < 19) {
							blens[border[index++]] = 0;
						}

						bb = 7;
						inftree.InflateTreeBits(blens, ref bb, ref tb, hufts, codec);
						index = 0;
						mode = InflateBlockMode.DTREE;
						goto case InflateBlockMode.DTREE;

					case InflateBlockMode.DTREE:
						while (true) {
							t = table;
							if (!(index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))) {
								break;
							}
							t = bb;

							while (bitsNum < t) {
								if (availIn != 0) {
									r = RCode.Okay;
								} else {
									return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
								}

								availIn--;
								bits |= codec.InputBuffer[nextIn++] << bitsNum;
								bitsNum += 8;
							}

							t = hufts[(tb + (bits & Constants.InflateMask[t])) * 3 + 1];
							int c = hufts[(tb + (bits & Constants.InflateMask[t])) * 3 + 2];

							if( c < 16 ) {
								bits >>= t; bitsNum -= t;
								blens[index++] = c;
							} else {
								// c == 16..18
								int i = c == 18 ? 7 : c - 14;
								int j = c == 18 ? 11 : 3;

								while( bitsNum < ( t + i ) ) {
									if( availIn != 0 ) {
										r = RCode.Okay;
									} else {
										return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
									}

									availIn--;
									bits |= codec.InputBuffer[nextIn++] << bitsNum;
									bitsNum += 8;
								}

								bits >>= t; bitsNum -= t;
								j += (bits & Constants.InflateMask[i]);
								bits >>= i; bitsNum -= i;

								i = index;
								if (i + j > 258 + (table & 0x1f) + ((table >> 5) & 0x1f) || (c == 16 && i < 1)) {
									throw new InvalidDataException( "invalid bit length repeat" );
								}

								c = (c == 16) ? blens[i-1] : 0;
								do {
									blens[i++] = c;
								} while (--j != 0);
								index = i;
							}
						}

						tb = -1;
						{
							int bl = 9;  // must be <= 9 for lookahead assumptions
							int bd = 6; // must be <= 9 for lookahead assumptions
							int tl = 0;
							int td = 0;
							inftree.InflateTreesDynamic(257 + (table & 0x1f), 1 + ((table >> 5) & 0x1f), blens, 
							                            ref bl, ref bd, ref tl, ref td, hufts);
							codes.Init(bl, bd, hufts, tl, hufts, td);
						}
						mode = InflateBlockMode.CODES;
						goto case InflateBlockMode.CODES;

					case InflateBlockMode.CODES:
						UpdateState( bits, bitsNum, availIn, nextIn, q );

						r = codes.Process(this, r);
						if (r != RCode.StreamEnd) return Flush(r);

						r = RCode.Okay;
						nextIn = codec.NextIn;
						availIn = codec.AvailableBytesIn;
						bits = bitb;
						bitsNum = bitk;
						q = writeAt;
						m = q < readAt ? readAt - q - 1 : end - q;

						if (last == 0) {
							mode = InflateBlockMode.TYPE;
							break;
						}
						mode = InflateBlockMode.DRY;
						goto case InflateBlockMode.DRY;

					case InflateBlockMode.DRY:
						writeAt = q;
						r = Flush(r);
						q = writeAt;
						m = q < readAt ? readAt - q - 1 : end - q;
						if (readAt != writeAt) {
							return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
						}
						mode = InflateBlockMode.DONE;
						goto case InflateBlockMode.DONE;

					case InflateBlockMode.DONE:
						return RanOutOfInput( bits, bitsNum, availIn, nextIn, q, RCode.StreamEnd );

					default:
						throw new InvalidOperationException( "Invalid inflate block mode: " + mode );
				}
			}
		}
		
		internal int RanOutOfInput( int bits, int bitsNum, int availIn, int nextIn, int q, int r ) {
			bitb = bits;
			bitk = bitsNum;
			codec.AvailableBytesIn = availIn;
			codec.NextIn = nextIn;
			writeAt = q;
			return Flush(r);
		}
		
		internal void UpdateState( int bits, int bitsNum, int availIn, int nextIn, int q ) {
			bitb = bits;
			bitk = bitsNum;
			codec.AvailableBytesIn = availIn;
			codec.NextIn = nextIn;
			writeAt = q;
		}

		internal void Free() {
			Reset();
			window = null;
			hufts = null;
		}

		// copy as much as possible from the sliding window to the output area
		internal int Flush( int r ) {
			for( int pass = 0; pass < 2; pass++ ) {
				int nBytes = pass == 0 ?
					// compute number of bytes to copy as far as end of window
					((readAt <= writeAt ? writeAt : end) - readAt) :
					// compute bytes to copy
					writeAt - readAt;

				// workitem 8870
				if (nBytes == 0) {
					if (r == RCode.BufferError)
						r = RCode.Okay;
					return r;
				}

				if (nBytes > codec.AvailableBytesOut)
					nBytes = codec.AvailableBytesOut;

				if (nBytes != 0 && r == RCode.BufferError)
					r = RCode.Okay;

				// update counters
				codec.AvailableBytesOut -= nBytes;

				// copy as far as end of window
				Array.Copy(window, readAt, codec.OutputBuffer, codec.NextOut, nBytes);
				codec.NextOut += nBytes;
				readAt += nBytes;

				// see if more to copy at beginning of window
				if (readAt == end && pass == 0) {
					// wrap pointers
					readAt = 0;
					if (writeAt == end)
						writeAt = 0;
				} else {
					pass++;
				}
			}
			return r;
		}
	}

	internal static class Constants {
		// And'ing with mask[n] masks the lower n bits
		internal static readonly int[] InflateMask = {
			0x00000000, 0x00000001, 0x00000003, 0x00000007,
			0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
			0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
			0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff };
	}


	sealed class InflateCodes
	{
		// waiting for "i:"=input, "o:"=output, "x:"=nothing
		const int START   = 0; // x: set up for LEN
		const int LEN     = 1; // i: get length/literal/eob next
		const int LENEXT  = 2; // i: getting length extra (have base)
		const int DIST    = 3; // i: get distance next
		const int DISTEXT = 4; // i: getting distance extra
		const int COPY    = 5; // o: copying bytes in window, waiting for space
		const int LIT     = 6; // o: got literal, waiting for output space
		const int WASH    = 7; // o: got eob, possibly still output waiting
		const int END     = 8; // x: got eob and all data flushed
		const int BADCODE = 9; // x: got error

		int mode;        // current inflate_codes mode

		// mode dependent information
		int len;

		int[] tree;      // pointer into tree
		int tree_index = 0;
		int need;        // bits needed

		int lit;

		// if EXT or COPY, where and how much
		int bitsToGet;   // bits to get for extra
		int dist;        // distance back to copy from

		byte lbits;      // ltree bits decoded per branch
		byte dbits;      // dtree bits decoder per branch
		int[] ltree;     // literal/length/eob tree
		int ltree_index; // literal/length/eob tree
		int[] dtree;     // distance tree
		int dtree_index; // distance tree

		internal void Init(int bl, int bd, int[] tl, int tl_index, int[] td, int td_index) {
			mode = START;
			lbits = (byte)bl;
			dbits = (byte)bd;
			ltree = tl;
			ltree_index = tl_index;
			dtree = td;
			dtree_index = td_index;
			tree = null;
		}

		internal int Process(InflateBlocks blocks, int r)
		{
			int tindex; // temporary pointer
			int e; // extra bits or operation   

			ZlibCodec z = blocks.codec;
			int nextIn = z.NextIn;// input data pointer
			int availIn = z.AvailableBytesIn; // bytes available there
			int bits = blocks.bitb; // bit buffer
			int bitsNum = blocks.bitk; // bits in bit buffer
			int q = blocks.writeAt;  // output window write pointer
			int m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q; // bytes to end of window or read pointer

			// process input and output based on current state
			while (true)
			{
				switch (mode)
				{
						// waiting for "i:"=input, "o:"=output, "x:"=nothing
					case START:  // x: set up for LEN
						if (m >= 258 && availIn >= 10) {
							blocks.UpdateState( bits, bitsNum, availIn, nextIn, q );
							r = InflateFast(lbits, dbits, ltree, ltree_index, dtree, dtree_index, blocks, z);

							nextIn = z.NextIn;
							availIn = z.AvailableBytesIn;
							bits = blocks.bitb;
							bitsNum = blocks.bitk;
							q = blocks.writeAt;
							m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;

							if (r != RCode.Okay)
							{
								mode = (r == RCode.StreamEnd) ? WASH : BADCODE;
								break;
							}
						}
						need = lbits;
						tree = ltree;
						tree_index = ltree_index;

						mode = LEN;
						goto case LEN;

					case LEN:  // i: get length/literal/eob next
						while (bitsNum < need) {
							if (availIn != 0) {
								r = RCode.Okay;
							} else {
								return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}
							availIn--;
							bits |= z.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						tindex = (tree_index + (bits & Constants.InflateMask[need])) * 3;

						bits >>= (tree[tindex + 1]);
						bitsNum -= (tree[tindex + 1]);

						e = tree[tindex];

						if (e == 0) {
							// literal
							lit = tree[tindex + 2];
							mode = LIT;
							break;
						}
						if ((e & 16) != 0) {
							// length
							bitsToGet = e & 15;
							len = tree[tindex + 2];
							mode = LENEXT;
							break;
						}
						if ((e & 64) == 0) {
							// next table
							need = e;
							tree_index = tindex / 3 + tree[tindex + 2];
							break;
						}
						if ((e & 32) != 0) {
							// end of block
							mode = WASH;
							break;
						}
						throw new InvalidDataException( "invalid literal/length code" );


					case LENEXT:  // i: getting length extra (have base)
						while (bitsNum < bitsToGet) {
							if (availIn != 0) {
								r = RCode.Okay;
							} else {
								return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}
							availIn--;
							bits |= z.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						len += (bits & Constants.InflateMask[bitsToGet]);

						bits >>= bitsToGet;
						bitsNum -= bitsToGet;

						need = dbits;
						tree = dtree;
						tree_index = dtree_index;
						mode = DIST;
						goto case DIST;

					case DIST:  // i: get distance next
						while (bitsNum < need) {
							if (availIn != 0) {
								r = RCode.Okay;
							} else {
								return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}
							availIn--;
							bits |= z.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						tindex = (tree_index + (bits & Constants.InflateMask[need])) * 3;

						bits >>= tree[tindex + 1];
						bitsNum -= tree[tindex + 1];

						e = tree[tindex];
						if ((e & 0x10) != 0) {
							// distance
							bitsToGet = e & 15;
							dist = tree[tindex + 2];
							mode = DISTEXT;
							break;
						}
						if ((e & 64) == 0) {
							// next table
							need = e;
							tree_index = tindex / 3 + tree[tindex + 2];
							break;
						}
						throw new InvalidDataException( "invalid distance code" );


					case DISTEXT:  // i: getting distance extra
						while (bitsNum < bitsToGet) {
							if (availIn != 0) {
								r = RCode.Okay;
							} else {
								return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
							}
							availIn--;
							bits |= z.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						dist += (bits & Constants.InflateMask[bitsToGet]);

						bits >>= bitsToGet;
						bitsNum -= bitsToGet;

						mode = COPY;
						goto case COPY;

					case COPY:  // o: copying bytes in window, waiting for space
						int f = q - dist; // pointer to copy strings from
						while (f < 0) {
							// modulo window size-"while" instead
							f += blocks.end; // of "if" handles invalid distances
						}
						while (len != 0) {
							if (m == 0) {
								if (q == blocks.end && blocks.readAt != 0) {
									q = 0;
									m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;
								}
								
								if (m == 0) {
									blocks.writeAt = q;
									r = blocks.Flush(r);
									q = blocks.writeAt;
									m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;

									if (q == blocks.end && blocks.readAt != 0) {
										q = 0;
										m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;
									}

									if (m == 0) {
										return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
									}
								}
							}

							blocks.window[q++] = blocks.window[f++];
							m--;

							if (f == blocks.end)
								f = 0;
							len--;
						}
						mode = START;
						break;

					case LIT:  // o: got literal, waiting for output space
						if (m == 0) {
							if (q == blocks.end && blocks.readAt != 0) {
								q = 0;
								m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;
							}
							
							if (m == 0) {
								blocks.writeAt = q;
								r = blocks.Flush(r);
								q = blocks.writeAt;
								m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;

								if (q == blocks.end && blocks.readAt != 0)
								{
									q = 0;
									m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;
								}
								if (m == 0) {
									return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
								}
							}
						}
						
						r = RCode.Okay;
						blocks.window[q++] = (byte)lit;
						m--;
						mode = START;
						break;

					case WASH:  // o: got eob, possibly more output
						if (bitsNum > 7)
						{
							// return unused byte, if any
							bitsNum -= 8;
							availIn++;
							nextIn--; // can always return one
						}

						blocks.writeAt = q;
						r = blocks.Flush(r);
						q = blocks.writeAt;
						m = q < blocks.readAt ? blocks.readAt - q - 1 : blocks.end - q;

						if (blocks.readAt != blocks.writeAt) {
							return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );
						}
						mode = END;
						goto case END;

					case END:
						r = RCode.StreamEnd;
						return blocks.RanOutOfInput( bits, bitsNum, availIn, nextIn, q, r );

					default:
						throw new InvalidDataException( "Encountered error: " + mode );
				}
			}
		}

		// Called with number of bytes left to write in window at least 258
		// (the maximum string length) and number of input bytes available
		// at least ten.  The ten bytes are six bytes for the longest length/
		// distance pair plus four bytes for overloading the bit buffer.
		internal int InflateFast(int bl, int bd, int[] tl, int tl_index, int[] td, int td_index, InflateBlocks s, ZlibCodec z)
		{
			int e;        // extra bits or operation
			int c;        // bytes to copy

			int nextIn = z.NextIn; // input data pointer
			int availIn = z.AvailableBytesIn; // bytes available there
			int bits = s.bitb; // bit buffer
			int bitsNum = s.bitk; // bits in bit buffer
			int q = s.writeAt; // output window write pointer
			int m = q < s.readAt ? s.readAt - q - 1 : s.end - q; // bytes to end of window or read pointer

			int ml = Constants.InflateMask[bl]; // mask for literal/length tree
			int md = Constants.InflateMask[bd]; // mask for distance tree

			// do until not enough input or output space for fast loop
			do {
				// assume called with m >= 258 && n >= 10
				// get literal/length code
				while (bitsNum < 20) {
					// max bits for literal/length code
					availIn--;
					bits |= z.InputBuffer[nextIn++] << bitsNum;
					bitsNum += 8;
				}

				int t = bits & ml; // temporary pointer
				int[] tp = tl;// temporary pointer
				int tp_index = tl_index;// temporary pointer
				int tp_index_t_3 = (tp_index + t) * 3;
				
				if ((e = tp[tp_index_t_3]) == 0) {
					bits >>= tp[tp_index_t_3 + 1];
					bitsNum -= tp[tp_index_t_3 + 1];

					s.window[q++] = (byte)tp[tp_index_t_3 + 2];
					m--;
					continue;
				}
				do {
					bits >>= tp[tp_index_t_3 + 1];
					bitsNum -= tp[tp_index_t_3 + 1];

					if ((e & 16) != 0) {
						e &= 15;
						c = tp[tp_index_t_3 + 2] + (bits & Constants.InflateMask[e]);

						bits >>= e; bitsNum -= e;

						// decode distance base of block to copy
						while (bitsNum < 15) {
							// max bits for distance code
							availIn--;
							bits |= z.InputBuffer[nextIn++] << bitsNum;
							bitsNum += 8;
						}

						t = bits & md;
						tp = td;
						tp_index = td_index;
						tp_index_t_3 = (tp_index + t) * 3;
						e = tp[tp_index_t_3];

						do {
							bits >>= (tp[tp_index_t_3 + 1]);
							bitsNum -= (tp[tp_index_t_3 + 1]);

							if ((e & 16) != 0) {
								// get extra bits to add to distance base
								e &= 15;
								while (bitsNum < e) {
									// get extra bits (up to 13)
									availIn--;
									bits |= z.InputBuffer[nextIn++] << bitsNum;
									bitsNum += 8;
								}

								int d = tp[tp_index_t_3 + 2] + (bits & Constants.InflateMask[e]); // distance back to copy from
								bits >>= e; bitsNum -= e;

								// do the copy
								int r = q - d; // copy source pointer
								m -= c;
								if (q >= d) {
									// offset before dest, just copy
									if (q - r > 0 && 2 > (q - r)) {
										s.window[q++] = s.window[r++]; // minimum count is three,
										s.window[q++] = s.window[r++]; // so unroll loop a little
									} else {
										Array.Copy(s.window, r, s.window, q, 2);
										q += 2; r += 2;
									}
									c -= 2;
								} else {
									// else offset after destination
									do {
										r += s.end; // force pointer in window
									} while (r < 0); // covers invalid distances
									e = s.end - r;
									if (c > e) {
										// if source crosses,
										c -= e; // wrapped copy
										if (q - r > 0 && e > (q - r)) {
											do {
												s.window[q++] = s.window[r++];
											} while (--e != 0);
										} else {
											Array.Copy(s.window, r, s.window, q, e);
											q += e; r += e; e = 0;
										}
										r = 0; // copy rest from start of window
									}
								}

								// copy all or what's left
								if( q - r > 0 && c > ( q - r ) ) {
									do {
										s.window[q++] = s.window[r++];
									} while ( --c != 0 );
								} else {
									Array.Copy( s.window, r, s.window, q, c );
									q += c; r += c;
									c = 0;
								}
								break;
							} else if ( ( e & 64 ) == 0 ) {
								t += tp[tp_index_t_3 + 2];
								t += (bits & Constants.InflateMask[e]);
								tp_index_t_3 = (tp_index + t) * 3;
								e = tp[tp_index_t_3];
							} else {
								throw new InvalidDataException( "invalid distance code" );
							}
						} while (true);
						break;
					}

					if ( ( e & 64 ) == 0 ) {
						t += tp[tp_index_t_3 + 2];
						t += (bits & Constants.InflateMask[e]);
						tp_index_t_3 = (tp_index + t) * 3;
						if ((e = tp[tp_index_t_3]) == 0)
						{
							bits >>= (tp[tp_index_t_3 + 1]); bitsNum -= (tp[tp_index_t_3 + 1]);
							s.window[q++] = (byte)tp[tp_index_t_3 + 2];
							m--;
							break;
						}
					} else if ( ( e & 32 ) != 0 ) {
						c = z.AvailableBytesIn - availIn;
						c = (bitsNum >> 3) < c ? bitsNum >> 3 : c;
						availIn += c;
						nextIn -= c;
						bitsNum -= (c << 3);

						s.UpdateState( bits, bitsNum, availIn, nextIn, q );
						return RCode.StreamEnd;
					} else {
						throw new InvalidDataException( "invalid literal/length code" );
					}
				} while (true);
			} while (m >= 258 && availIn >= 10);

			// not enough input or output--restore pointers and return
			c = z.AvailableBytesIn - availIn;
			c = (bitsNum >> 3) < c ? bitsNum >> 3 : c;
			availIn += c;
			nextIn -= c;
			bitsNum -= (c << 3);

			s.UpdateState( bits, bitsNum, availIn, nextIn, q );
			return RCode.Okay;
		}
	}


	internal sealed class InflateManager
	{
		bool done = false;
		ZlibCodec _codec; // pointer back to this zlib stream

		int wbits; // log2(window size)  (8..15, defaults to 15)
		InflateBlocks blocks; // current inflate_blocks state

		internal void Reset() {
			done = false;
			blocks.Reset();
		}

		internal void End() {
			if (blocks != null)
				blocks.Free();
			blocks = null;
		}

		internal void Initialize( ZlibCodec codec, int w ) {
			_codec = codec;
			blocks = null;
			
			wbits = w;
			blocks = new InflateBlocks( codec, 1 << w );
			Reset();
		}

		internal int Inflate() {
			if (_codec.InputBuffer == null)
				throw new InvalidOperationException("InputBuffer is null. ");

			int r = RCode.BufferError;
			if( !done ) {
				r = blocks.Process(r);
				if( r == RCode.DataError ) {
					throw new InvalidDataException( "Bad state" );
				}

				if (r != RCode.StreamEnd)
					return r;

				blocks.Reset();
				done = true;
			}
			return RCode.StreamEnd;
		}
	}
}
#endif