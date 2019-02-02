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
using System.Runtime.CompilerServices;
using csogg;

namespace csvorbis
{
	class Mdct
	{

		//static private float cPI3_8 = 0.38268343236508977175f;
		//static private float cPI2_8 = 0.70710678118654752441f;
		//static private float cPI1_8 = 0.92387953251128675613f;

		int n;
		int log2n;
		
		float[] trig;
		int[] bitrev;

		float scale;

		internal void init(int n)
		{
			bitrev = new int[n/4];
			trig = new float[n+n/4];

			int n2 = (int)((uint)n >> 1);
			log2n = (int)Math.Round(Math.Log(n)/Math.Log(2));
			this.n = n;


			int AE = 0;
			int AO = 1;
			int BE = AE+n/2;
			int BO = BE+1;
			int CE = BE+n/2;
			int CO = CE+1;
			// trig lookups...
			for(int i = 0;i<n/4;i++)
			{
				trig[AE+i*2] = (float)Math.Cos((Math.PI/n)*(4*i));
				trig[AO+i*2] = (float)-Math.Sin((Math.PI/n)*(4*i));
				trig[BE+i*2] = (float)Math.Cos((Math.PI/(2*n))*(2*i+1));
				trig[BO+i*2] = (float)Math.Sin((Math.PI/(2*n))*(2*i+1));
			}
			for(int i = 0;i<n/8;i++)
			{
				trig[CE+i*2] = (float)Math.Cos((Math.PI/n)*(4*i+2));
				trig[CO+i*2] = (float)-Math.Sin((Math.PI/n)*(4*i+2));
			}

			{
				int mask = (1<<(log2n-1))-1;
				int msb = 1<<(log2n-2);
				for(int i = 0;i<n/8;i++)
				{
					int acc = 0;
					for(int j = 0; (((uint)msb) >> j)  !=  0; j++)
						if(((((uint)msb>>j))&i)  !=  0)
							acc |=  1 << j;
					bitrev[i*2] = ((~acc)&mask);
					//	bitrev[i*2] = ((~acc)&mask)-1;
					bitrev[i*2+1] = acc;
				}
			}
			scale = 4.0f/n;
		}

		float[] _x = new float[1024];
		float[] _w = new float[1024];
		
		internal void backward(float[] fin, float[] fout)
		{
			if(_x.Length < n/2){_x = new float[n/2];}
			if(_w.Length < n/2){_w = new float[n/2];}
			float[] x = _x;
			float[] w = _w;
			int n2 = (int)((uint)n >> 1);
			int n4 = (int)((uint)n >> 2);
			int n8 = (int)((uint)n >> 3);

			// rotate + step 1
			int inO = 1;
			int xO = 0;
			int A = n2;

			for(int i = 0;i<n8;i++) {
				A -= 2;
				x[xO++] = -fin[inO+2]*trig[A+1] - fin[inO]*trig[A];
				x[xO++] =  fin[inO]*trig[A+1] - fin[inO+2]*trig[A];
				inO += 4;
			}

			inO = n2-4;

			for(int i = 0;i<n8;i++) {
				A -= 2;
				x[xO++] = fin[inO]*trig[A+1] + fin[inO+2]*trig[A];
				x[xO++] = fin[inO]*trig[A] - fin[inO+2]*trig[A+1];
				inO -= 4;
			}

			float[] xxx = mdct_kernel(x,w,n,n2,n4,n8);
			int xx = 0;

			// step 8
			int B = n2;
			int o1 = n4,o2 = o1-1;
			int o3 = n4+n2,o4 = o3-1;
			
			for(int i = 0;i<n4;i++)
			{
				float temp1 =  (xxx[xx] * trig[B+1] - xxx[xx+1] * trig[B]);
				float temp2 = -(xxx[xx] * trig[B] + xxx[xx+1] * trig[B+1]);
				
				fout[o1] = -temp1;
				fout[o2] =  temp1;
				fout[o3] =  temp2;
				fout[o4] =  temp2;

				o1++;
				o2--;
				o3++;
				o4--;
				xx += 2;
				B += 2;
			}
		}
		internal float[] mdct_kernel(float[] x, float[] w,
		                             int n, int n2, int n4, int n8)
		{
			// step 2

			int xA = n4;
			int xB = 0;
			int w2 = n4;
			int A = n2;

			for(int i = 0;i<n4;)
			{
				float xx0 = x[xA] - x[xB];
				w[w2+i] = x[xA++] + x[xB++];
				float xx1 = x[xA] - x[xB];
				A -= 4;

				w[i++] =    xx0 * trig[A] + xx1 * trig[A+1];
				w[i] =      xx1 * trig[A] - xx0 * trig[A+1];

				w[w2+i] = x[xA++]+x[xB++];
				i++;
			}

			// step 3
			for(int i = 0;i<log2n-3;i++)
			{
				int k0 = (int)((uint)n >> (i+2));
				int k1 = 1 << (i+3);
				int wbase = n2-2;

				A = 0;
				float[] temp;

				for(int r = 0; r<((uint)k0>>2); r++)
				{
					int w1 = wbase;
					w2 = w1-(k0>>1);
					float AEv =  trig[A],wA;
					float AOv =  trig[A+1],wB;
					wbase -= 2;
					
					k0++;
					for(int s = 0;s<(2<<i);s++)
					{
						wB      = w[w1]   -w[w2];
						x[w1]   = w[w1]   +w[w2];

						wA      = w[++w1] -w[++w2];
						x[w1]   = w[w1]   +w[w2];
						
						x[w2]   = wA*AEv  - wB*AOv;
						x[w2-1] = wB*AEv  + wA*AOv;

						w1 -= k0;
						w2 -= k0;
					}
					k0--;
					A += k1;
				}

				temp = w;
				w = x;
				x = temp;
			}

			// step 4, 5, 6, 7
			int C = n;
			int bit = 0;
			int x1 = 0;
			int x2 = n2-1;

			for(int i = 0;i<n8;i++)
			{
				int t1 = bitrev[bit++];
				int t2 = bitrev[bit++];

				float wA = w[t1]-w[t2+1];
				float wB = w[t1-1]+w[t2];
				float wC = w[t1]+w[t2+1];
				float wD = w[t1-1]-w[t2];

				float wACE = wA* trig[C];
				float wBCE = wB* trig[C++];
				float wACO = wA* trig[C];
				float wBCO = wB* trig[C++];
				
				x[x1++] = ( wC+wACO+wBCE)*.5f;
				x[x2--] = (-wD+wBCO-wACE)*.5f;
				x[x1++] = ( wD+wBCO-wACE)*.5f;
				x[x2--] = ( wC-wACO-wBCE)*.5f;
			}
			return(x);
		}
	}
}