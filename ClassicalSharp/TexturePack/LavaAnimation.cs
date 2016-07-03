// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
using System;
using System.Security.Cryptography;
using ClassicalSharp.Generator;
using ClassicalSharp;

namespace ClassicalSharp {
	public unsafe class LavaAnimation {
		const int size = 16;
		float[] flameHeat = new float[size * size];
		float[] potHeat = new float[size * size];
		float[] soupHeat = new float[size * size];
		JavaRandom rnd = null;
		
		public void Tick( FastBitmap output ) {
			if( rnd == null )
				rnd = new JavaRandom( new Random().Next() );
			Step();
			Output( output );
		}
		
		void Step() {
			int index = 0;
			for( int y = 0; y < size; y++ )
				for( int x = 0; x < size; x++ )
			{
				float localSoupHeat = 0;
				for( int i = 0; i < 9; i++ ) {
					int xx = x + (i % 3 - 1) + (int)(1.2 * Math.Sin( y * 22.5 * Utils.Deg2Rad ));
					int yy = y + (i / 3 - 1) + (int)(1.2 * Math.Sin( x * 22.5 * Utils.Deg2Rad ));
					localSoupHeat += soupHeat[Index( xx, yy )];
				}
				
				float localPotHeat =
					potHeat[Index( x, y )] + potHeat[Index( x, y + 1 )] +
					potHeat[Index( x + 1, y )] + potHeat[Index( x + 1, y + 1 )];
				
				soupHeat[index] = localSoupHeat / 10 + localPotHeat / 4 * 0.8f;
				potHeat[index] += flameHeat[index] * 0.01f;
				if( potHeat[index] < 0 ) potHeat[index] = 0;
				flameHeat[index] -= 0.06f;
				
				if( rnd.NextFloat() <= 0.005f )
					flameHeat[index] = 1.5f;
				index++;
			}
		}
		
		void Output( FastBitmap output ) {
			int index = 0;
			for( int y = 0; y < size; y++ ) {
				int* row = output.GetRowPtr( y );
				for( int x = 0; x < size; x++ ) {
					float col = 2 * soupHeat[index];
					Utils.Clamp( ref col, 0, 1 );
					float r = col * 100 + 155;
					float g = col * col * 255;
					float b = col * col * col * col * 128;
					row[x] = 255 << 24 | (byte)r << 16 | (byte)g << 8 | (byte)b;
					index++;
				}
			}
		}
		
		static int Index( int x, int y ) {
			return (y & 0xF) << 4 | (x & 0xF);
		}
	}
}