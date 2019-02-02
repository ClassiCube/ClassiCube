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
using System.Text;
using csogg;

namespace csvorbis
{
	class InfoMode
	{
		internal int blockflag;
		internal int windowtype;
		internal int transformtype;
		internal int mapping;
	}
	
	public class Info
	{
		private const int OV_ENOTAUDIO = -135;

		public int version, channels, rate;
		
		// Vorbis supports only short and long blocks, but allows the
		// encoder to choose the sizes
		internal int[] blocksizes = new int[2];

		// modes are the primary means of supporting on-the-fly different
		// blocksizes, different channel mappings (LR or mid-side),
		// different residue backends, etc.  Each mode consists of a
		// blocksize flag and a mapping (along with the mapping setup

		internal int modes, maps, times, floors, residues, books;

		internal InfoMode[] mode_param = null;

		internal FuncMapping[] map_funcs = null;
		internal Object[] map_param = null;

		internal Floor1[] floor_funcs = null;
		internal Object[] floor_param = null;

		internal FuncResidue[] residue_funcs = null;
		internal Object[] residue_param = null;

		internal StaticCodeBook[] book_param = null;

		// used by synthesis, which has a full, alloced vi
		public void init() {
			rate = 0;
		}

		public void clear() {
			mode_param = null;
			map_param = null;
			floor_param = null;
			residue_param = null;
			book_param = null;
		}

		// Header packing/unpacking
		void unpack_info(csBuffer opb) {
			version = opb.read(32);
			channels = opb.read(8);
			rate = opb.read(32);

			opb.read(32); // bitrate_upper
			opb.read(32); // bitrate_nominal
			opb.read(32); // bitrate_lower

			blocksizes[0] = 1<<opb.read(4);
			blocksizes[1] = 1<<opb.read(4);
			opb.read(1);
		}
		
		void unpack_books(csBuffer opb) {
			books = opb.read(8) + 1;
			book_param = new StaticCodeBook[books];
			for(int i = 0;i < books;i++) {
				book_param[i] = new StaticCodeBook();
				book_param[i].unpack(opb);
			}
			
			times = opb.read(6) + 1;
			for(int i = 0;i<times;i++)
				opb.read(16);

			floors = opb.read(6) + 1;
			floor_funcs = new Floor1[floors];
			floor_param = new object[floors];
			for(int i = 0;i<floors;i++) {
				int type = opb.read(16);
				if (type != 1) throw new csorbisException("floor type must be 1");
				floor_funcs[i] = new Floor1();			
				floor_param[i] = floor_funcs[i].unpack(this,opb);
			}
			
			residues = opb.read(6) + 1;
			residue_funcs = new FuncResidue[residues];
			residue_param = new object[residues];
			for(int i = 0;i<residues;i++) {
				int type = opb.read(16);
				if (type > 2) throw new csorbisException("residue type must be <= 2");
				residue_funcs[i] = FuncResidue.make(type);
				residue_param[i] = residue_funcs[i].unpack(this,opb);
			}

			maps = opb.read(6) + 1;
			map_funcs = new FuncMapping[maps];
			map_param = new object[maps];
			for(int i = 0;i<maps;i++) {
				int type = opb.read(16);
				if (type != 0) throw new csorbisException("mapping type must be 0");
				map_funcs[i] = new Mapping0();
				map_param[i] = map_funcs[i].unpack(this,opb);
			}
			
			modes = opb.read(6) + 1;
			mode_param = new InfoMode[modes];
			for(int i = 0;i<modes;i++) {
				mode_param[i] = new InfoMode();
				mode_param[i].blockflag = opb.read(1);
				mode_param[i].windowtype = opb.read(16);
				mode_param[i].transformtype = opb.read(16);
				mode_param[i].mapping = opb.read(8);
			}
			opb.read(1);
		}

		public void synthesis_headerin(Comment vc, Packet op) {
			if(op == null) return;
			
			csBuffer opb = new csBuffer();
			opb.readinit(op.packet_base, op.packet, op.bytes);
			byte[] buffer = new byte[6];
			int packtype = opb.read(8);
			opb.read(buffer, 6);
			
			if(buffer[0] != 'v' || buffer[1] != 'o' || buffer[2] != 'r' ||
			   buffer[3] != 'b' || buffer[4] != 'i' || buffer[5] != 's') {
				throw new InvalidOperationException("Expected vorbis header");
			}
			
			switch(packtype) {
				case 0x01: // least significant *bit* is read first
					unpack_info(opb);
					break;
				case 0x03: // least significant *bit* is read first
					vc.unpack(opb);
					break;
				case 0x05: // least significant *bit* is read first
					unpack_books(opb);
					break;
				default:
					// Not a valid vorbis header type
					break;
			}
		}

		public int blocksize(Packet op) {
			csBuffer opb = new csBuffer();
			opb.readinit(op.packet_base, op.packet, op.bytes);

			/* Check the packet type */
			if(opb.read(1) != 0)
				return(OV_ENOTAUDIO);
			
			int modebits = VUtils.ilog2(modes);
			int mode = opb.read(modebits);
			return blocksizes[mode_param[mode].blockflag];
		}

		public String toString()
		{
			return "version:" + version + ", channels:" + channels
				+ ", rate:" + rate;
		}
	}
}
