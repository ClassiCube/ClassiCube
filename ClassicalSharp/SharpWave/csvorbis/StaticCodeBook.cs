/* csvorbis
 * Copyright (C) 2000 ymnk, JCraft,Inc.
 *
 * Written by: 2000 ymnk<ymnk@jcraft.com>
 * Ported to C# from JOrbis by: Mark Crichton <crichton@gimp.org>
 *
 * Thanks go to the JOrbis team, for licencing the code under the
 * LGPL, making my job a lot easier.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
   
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


using System;
using csogg;

namespace csvorbis
{
	class StaticCodeBook
	{
		internal int   dim;            // codebook dimensions (elements per vector)
		internal int   entries;        // codebook entries
		internal int[] lengthlist;     // codeword lengths in bits

		// mapping
		internal int   maptype;        // 0 = none
		// 1 = implicitly populated values from map column
		// 2 = listed arbitrary values

		// The below does a linear, single monotonic sequence mapping.
		internal int   q_min;       // packed 32 bit float; quant value 0 maps to minval
		internal int   q_delta;     // packed 32 bit float; val 1 - val 0 = =  delta
		internal int   q_quant;     // bits: 0 < quant  <=  16
		internal int   q_sequencep; // bitflag

		// additional information for log (dB) mapping; the linear mapping
		// is assumed to actually be values in dB.  encodebias is used to
		// assign an error weight to 0 dB. We have two additional flags:
		// zeroflag indicates if entry zero is to represent -Inf dB; negflag
		// indicates if we're to represent negative linear values in a
		// mirror of the positive mapping.

		internal int[] quantlist;  // map = =  1: (int)(entries/dim) element column map
		// map = =  2: list of dim*entries quantized entry vals

		internal StaticCodeBook(){}
		internal StaticCodeBook(int dim, int entries, int[] lengthlist,
		                        int maptype, int q_min, int q_delta,
		                        int q_quant, int q_sequencep, int[] quantlist,
		                        //EncodeAuxNearestmatch nearest_tree,
		                        Object nearest_tree,
		                        // EncodeAuxThreshmatch thresh_tree,
		                        Object thresh_tree
		                       ) : this()
		{
			this.dim = dim; this.entries = entries; this.lengthlist = lengthlist;
			this.maptype = maptype; this.q_min = q_min; this.q_delta = q_delta;
			this.q_quant = q_quant; this.q_sequencep = q_sequencep;
			this.quantlist = quantlist;
		}

		// unpacks a codebook from the packet buffer into the codebook struct,
		// readies the codebook auxiliary structures for decode
		internal int unpack(csBuffer opb)
		{
			int i;
			opb.read(24);
			// first the basic parameters
			dim = opb.read(16);
			entries = opb.read(24);

			// codeword ordering.... length ordered or unordered?
			switch(opb.read(1))
			{
				case 0:
					// unordered
					lengthlist = new int[entries];

					// allocated but unused entries?
					if(opb.read(1) != 0)
					{
						// yes, unused entries

						for(i = 0;i<entries;i++)
						{
							if(opb.read(1) != 0)
							{
								int num = opb.read(5);
								lengthlist[i] = num+1;
							}
							else
							{
								lengthlist[i] = 0;
							}
						}
					}
					else
					{
						// all entries used; no tagging
						for(i = 0;i<entries;i++)
						{
							int num = opb.read(5);
							lengthlist[i] = num+1;
						}
					}
					break;
				case 1:
					// ordered
					{
						int length = opb.read(5)+1;
						lengthlist = new int[entries];

						for(i = 0;i<entries;)
						{
							int num = opb.read(VUtils.ilog(entries-i));
							for(int j = 0;j<num;j++,i++)
							{
								lengthlist[i] = length;
							}
							length++;
						}
					}
					break;
				default:
					// EOF
					return(-1);
			}
			
			// Do we have a mapping to unpack?
			switch((maptype = opb.read(4)))
			{
				case 0:
					// no mapping
					break;
				case 1:
				case 2:
					// implicitly populated value mapping
					// explicitly populated value mapping
					q_min = opb.read(32);
					q_delta = opb.read(32);
					q_quant = opb.read(4)+1;
					q_sequencep = opb.read(1);

					{
						int quantvals = 0;
						switch(maptype)
						{
							case 1:
								quantvals = maptype1_quantvals();
								break;
							case 2:
								quantvals = entries*dim;
								break;
						}
						
						// quantized values
						quantlist = new int[quantvals];
						for(i = 0;i<quantvals;i++)
							quantlist[i] = opb.read(q_quant);
					}
					break;
				default:
					return(-1);
			}
			return(0);
		}

		// there might be a straightforward one-line way to do the below
		// that's portable and totally safe against roundoff, but I haven't
		// thought of it.  Therefore, we opt on the side of caution
		internal int maptype1_quantvals()
		{
			int vals = (int)(Math.Floor(Math.Pow(entries,1.0/dim)));

			// the above *should* be reliable, but we'll not assume that FP is
			// ever reliable when bitstream sync is at stake; verify via integer
			// means that vals really is the greatest value of dim for which
			// vals^b->bim  <=  b->entries
			// treat the above as an initial guess
			while(true)
			{
				int acc = 1;
				int acc1 = 1;
				for(int i = 0;i<dim;i++)
				{
					acc *= vals;
					acc1 *= vals + 1;
				}
				if(acc <= entries && acc1>entries){	return(vals); }
				else
				{
					if(acc>entries){ vals--; }
					else{ vals++; }
				}
			}
		}

		// unpack the quantized list of values for encode/decode
		// we need to deal with two map types: in map type 1, the values are
		// generated algorithmically (each column of the vector counts through
		// the values in the quant vector). in map type 2, all the values came
		// in in an explicit list.  Both value lists must be unpacked
		internal float[] unquantize()
		{

			if(maptype == 1 || maptype == 2)
			{
				int quantvals;
				float mindel = float32_unpack(q_min);
				float delta = float32_unpack(q_delta);
				float[] r = new float[entries*dim];

				//System.err.println("q_min = "+q_min+", mindel = "+mindel);

				// maptype 1 and 2 both use a quantized value vector, but
				// different sizes
				switch(maptype)
				{
					case 1:
						// most of the time, entries%dimensions = =  0, but we need to be
						// well defined.  We define that the possible vales at each
						// scalar is values = =  entries/dim.  If entries%dim  !=  0, we'll
						// have 'too few' values (values*dim<entries), which means that
						// we'll have 'left over' entries; left over entries use zeroed
						// values (and are wasted).  So don't generate codebooks like that
						quantvals = maptype1_quantvals();
						for(int j = 0;j<entries;j++)
						{
							float last = 0.0f;
							int indexdiv = 1;
							for(int k = 0;k<dim;k++)
							{
								int index = (j/indexdiv)%quantvals;
								float val = quantlist[index];
								val = Math.Abs(val)*delta+mindel+last;
								if(q_sequencep != 0)last = val;
								r[j*dim+k] = val;
								indexdiv *= quantvals;
							}
						}
						break;
					case 2:
						for(int j = 0;j<entries;j++)
						{
							float last = 0.0f;
							for(int k = 0;k<dim;k++)
							{
								float val = quantlist[j*dim+k];
								val = Math.Abs(val)*delta+mindel+last;
								if(q_sequencep != 0)last = val;
								r[j*dim+k] = val;
							}
						}
						break;
					default:
						break;
				}
				return(r);
			}
			return(null);
		}

		internal static float float32_unpack(int val)
		{
			float mant = val&0x1fffff;
			float sign = val&0x80000000;
			float exp  = (uint)(val&0x7fe00000) >> 21;
			if((val&0x80000000) != 0)
				mant = -mant;
			return ldexp(mant, (int)exp - 788);
		}

		internal static float ldexp(float foo, int e)
		{
			return (float)(foo*Math.Pow(2, e));
		}
	}
}
