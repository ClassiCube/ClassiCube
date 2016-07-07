// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
using System;
using System.Security.Cryptography;
using ClassicalSharp.Generator;
using ClassicalSharp;

namespace ClassicalSharp {
	public unsafe class LavaAnimation {
		public const int Size = 16;
		float[] flameHeat = new float[Size * Size];
		float[] potHeat = new float[Size * Size];
		float[] soupHeat = new float[Size * Size];
		JavaRandom rnd = null;
		
		public unsafe void Tick( int* output ) {
			if( rnd == null )
				rnd = new JavaRandom( new Random().Next() );
			Step();
			Output( output );
		}
		
		void Step() {
			int i = 0;
			for( int y = 0; y < Size; y++ )
				for( int x = 0; x < Size; x++ )
			{
				float localSoupHeat = 0;
				int xOffset = (int)(1.2 * Math.Sin( y * 22.5 * Utils.Deg2Rad ));
				int yOffset = (int)(1.2 * Math.Sin( x * 22.5 * Utils.Deg2Rad ));
				for( int j = 0; j < 9; j++ ) {
					int xx = x + (j % 3 - 1) + xOffset;
					int yy = y + (j / 3 - 1) + yOffset;
					localSoupHeat += soupHeat[Index( xx, yy )];
				}
				
				float localPotHeat =
					potHeat[Index( x, y )] + potHeat[Index( x, y + 1 )] +
					potHeat[Index( x + 1, y )] + potHeat[Index( x + 1, y + 1 )];
				
				soupHeat[i] = localSoupHeat / 10 + localPotHeat / 4 * 0.8f;
				potHeat[i] += flameHeat[i] * 0.01f;
				if( potHeat[i] < 0 ) potHeat[i] = 0;
				flameHeat[i] -= 0.06f;
				
				if( rnd.NextFloat() <= 0.005f )
					flameHeat[i] = 1.5f;
				i++;
			}
		}
		
		void Output( int* ptr ) {
			for( int i = 0; i < soupHeat.Length; i++) {
				float col = 2 * soupHeat[i];
				col = col < 0 ? 0 : col;
				col = col > 1 ? 1 : col;
				
				float r = col * 100 + 155;
				float g = col * col * 255;
				float b = col * col * col * col * 128;
				*ptr = 255 << 24 | (byte)r << 16 | (byte)g << 8 | (byte)b;
				ptr++;
			}
		}
		
		static int Index( int x, int y ) {
			return (y & 0xF) << 4 | (x & 0xF);
		}
	}
}