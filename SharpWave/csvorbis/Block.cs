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
	public class Block
	{
		//necessary stream state for linking to the framing abstraction
		internal float[][] pcm = new float[0][]; // this is a pointer into local storage
		internal csBuffer opb = new csBuffer();
		
		internal int lW;
		internal int W;
		internal int nW;
		internal int pcmend;
		internal int mode;

		internal int eofflag;
		internal long granulepos;
		internal long sequence;
		internal DspState vd; // For read-only access of configuration

		public void init(DspState vd) {
			this.vd = vd;
		}

		public int synthesis(Packet op)
		{
			Info vi = vd.vi;
			opb.readinit(op.packet_base, op.packet, op.bytes);
			opb.read(1);
			// read our mode and pre/post windowsize
			mode = opb.read(vd.modebits);
			W = vi.mode_param[mode].blockflag;
			if(W != 0) {
				lW = opb.read(1); nW = opb.read(1);
			} else {
				lW = 0; nW = 0;
			}
			
			// more setup
			granulepos = op.granulepos;
			sequence = op.packetno-3; // first block is third packet
			eofflag = op.e_o_s;

			// alloc pcm passback storage
			pcmend = vi.blocksizes[W];
			if(pcm.Length<vi.channels)
				pcm = new float[vi.channels][];
			
			for(int i = 0; i < vi.channels; i++) {
				if( pcm[i] == null || pcm[i].Length < pcmend ) {
					pcm[i] = new float[pcmend];
				} else {
					for(int j = 0; j < pcmend; j++)
						pcm[i][j] = 0;
				}
			}

			// unpack_header enforces range checking
			int type = vi.map_type[vi.mode_param[mode].mapping];
			return(FuncMapping.mapping_P[type].inverse(this, vd.mode[mode]));
		}
	}
}
