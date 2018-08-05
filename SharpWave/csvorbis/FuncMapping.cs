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
	abstract class FuncMapping
	{
		public static FuncMapping[] mapping_P = {new Mapping0()};

		public abstract Object unpack(Info info , csBuffer buffer);
		public abstract Object look(DspState vd, InfoMode vm, Object m);
		public abstract int inverse(Block vd, Object lm);
	}
	
	class Mapping0 : FuncMapping
	{
		public override Object look(DspState vd, InfoMode vm, Object m)
		{
			Info vi = vd.vi;
			LookMapping0 looks = new LookMapping0();
			InfoMapping0 info = looks.map = (InfoMapping0)m;
			looks.mode = vm;
			
			looks.floor_look = new Object[info.submaps];
			looks.residue_look = new Object[info.submaps];

			looks.floor_func = new FuncFloor[info.submaps];
			looks.residue_func = new FuncResidue[info.submaps];
			
			for(int i = 0;i<info.submaps;i++)
			{
				int timenum = info.timesubmap[i];
				int floornum = info.floorsubmap[i];
				int resnum = info.residuesubmap[i];
				
				looks.floor_func[i] = FuncFloor.floor_P[vi.floor_type[floornum]];
				looks.floor_look[i] = looks.floor_func[i].
					look(vd,vm,vi.floor_param[floornum]);

				looks.residue_func[i] = FuncResidue.residue_P[vi.residue_type[resnum]];
				looks.residue_look[i] = looks.residue_func[i].
					look(vd,vm,vi.residue_param[resnum]);
			}

			looks.ch = vi.channels;
			return(looks);
		}

		public override Object unpack(Info vi, csBuffer opb) {
			InfoMapping0 info = new InfoMapping0();
			info.submaps = opb.read(1) != 0
				? opb.read(4) + 1 : 1;
			
			if(opb.read(1) != 0) {
				info.coupling_steps = opb.read(8)+1;

				for(int i = 0;i<info.coupling_steps;i++) {
					info.coupling_mag[i] = opb.read(VUtils.ilog2(vi.channels));
					info.coupling_ang[i] = opb.read(VUtils.ilog2(vi.channels));
				}
			}
			opb.read(2);

			if(info.submaps>1) {
				for(int i = 0;i<vi.channels;i++)
					info.chmuxlist[i] = opb.read(4);
			}

			for(int i = 0;i<info.submaps;i++)
			{
				info.timesubmap[i] = opb.read(8);
				info.floorsubmap[i] = opb.read(8);
				info.residuesubmap[i] = opb.read(8);
			}
			return info;
		}


		float[][] pcmbundle = null;
		int[] zerobundle = null;
		int[] nonzero = null;
		Object[] floormemo = null;

		public override int inverse(Block vb, Object l)
		{
			//System.err.println("Mapping0.inverse");
			DspState vd = vb.vd;
			Info vi = vd.vi;
			LookMapping0 look = (LookMapping0)l;
			InfoMapping0 info = look.map;
			InfoMode mode = look.mode;
			int n = vb.pcmend = vi.blocksizes[vb.W];

			float[] window = vd.wnd[vb.W][vb.lW][vb.nW];
			if(pcmbundle == null || pcmbundle.Length<vi.channels)
			{
				pcmbundle = new float[vi.channels][];
				nonzero = new int[vi.channels];
				zerobundle = new int[vi.channels];
				floormemo = new Object[vi.channels];
			}

			// recover the spectral envelope; store it in the PCM vector for now
			for(int i = 0;i<vi.channels;i++)
			{
				float[] pcm = vb.pcm[i];
				int submap = info.chmuxlist[i];

				floormemo[i] = look.floor_func[submap].inverse1(vb,look.
				                                                floor_look[submap],
				                                                floormemo[i]
				                                               );
				if(floormemo[i] != null)
					nonzero[i] = 1;
				else
					nonzero[i] = 0;
				for(int j = 0; j<n/2; j++)
					pcm[j] = 0;
			}

			for(int i = 0; i<info.coupling_steps; i++)
			{
				if(nonzero[info.coupling_mag[i]] != 0 || nonzero[info.coupling_ang[i]] != 0) {
					nonzero[info.coupling_mag[i]] = 1;
					nonzero[info.coupling_ang[i]] = 1;
				}
			}

			// recover the residue, apply directly to the spectral envelope
			for(int i = 0;i<info.submaps;i++) {
				int ch_in_bundle = 0;
				for(int j = 0;j<vi.channels;j++) {
					if(info.chmuxlist[j] != i) continue;
					
					zerobundle[ch_in_bundle] = nonzero[j] != 0 ? 1 : 0;
					pcmbundle[ch_in_bundle++] = vb.pcm[j];
				}

				look.residue_func[i].inverse(vb,look.residue_look[i],
				                             pcmbundle,zerobundle,ch_in_bundle);
			}
			InverseCoupling(n, vb, info);

			// compute and apply spectral envelope 
			for(int i = 0;i<vi.channels;i++) {
				float[] pcm = vb.pcm[i];
				int submap = info.chmuxlist[i];
				look.floor_func[submap].inverse2(vb,look.floor_look[submap],floormemo[i],pcm);
			}

			// transform the PCM data; takes PCM vector, vb; modifies PCM vector
			// only MDCT right now....
			for(int i = 0;i<vi.channels;i++) {
				float[] pcm = vb.pcm[i];
				((Mdct)vd.transform[vb.W]).backward(pcm,pcm);
			}
			
			// window the data
			for(int i = 0;i<vi.channels;i++) {
				float[] pcm = vb.pcm[i];
				if(nonzero[i] != 0) {
					for(int j = 0;j<n;j++)
						pcm[j] *= window[j];
				} else {
					for(int j = 0;j<n;j++)
						pcm[j] = 0.0f;
				}
			}
			return(0);
		}

		void InverseCoupling(int n, Block vb, InfoMapping0 info) {
			for( int i = info.coupling_steps - 1;i >= 0; i-- ) {
				float[] pcmM = vb.pcm[info.coupling_mag[i]];
				float[] pcmA = vb.pcm[info.coupling_ang[i]];

				for( int j = 0; j < n / 2; j++ ) {
					float mag = pcmM[j];
					float ang = pcmA[j];

					if( mag > 0 ) {
						if( ang > 0 ) {
							pcmM[j] = mag;
							pcmA[j] = mag-ang;
						} else {
							pcmA[j] = mag;
							pcmM[j] = mag+ang;
						}
					} else {
						if( ang > 0 ) {
							pcmM[j] = mag;
							pcmA[j] = mag+ang;
						} else {
							pcmA[j] = mag;
							pcmM[j] = mag-ang;
						}
					}
				}
			}
		}
	}

	class InfoMapping0
	{
		internal int   submaps;  //  <=  16
		internal int[] chmuxlist = new int[256];   // up to 256 channels in a Vorbis stream
		
		internal int[] timesubmap = new int[16];   // [mux]
		internal int[] floorsubmap = new int[16];  // [mux] submap to floors
		internal int[] residuesubmap = new int[16];// [mux] submap to residue
		internal int[] psysubmap = new int[16];    // [mux]; encode only

		internal int   coupling_steps;
		internal int[] coupling_mag = new int[256];
		internal int[] coupling_ang = new int[256];

		internal void free()
		{
			chmuxlist = null;
			timesubmap = null;
			floorsubmap = null;
			residuesubmap = null;
			psysubmap = null;

			coupling_mag = null;
			coupling_ang = null;
		}
	}

	class LookMapping0
	{
		internal InfoMode mode;
		internal InfoMapping0 map;
		internal Object[] floor_look;
		internal Object[] residue_look;

		internal FuncFloor[] floor_func;
		internal FuncResidue[] residue_func;

		internal int ch;
	}
}