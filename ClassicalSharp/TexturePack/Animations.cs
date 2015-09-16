using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.TexturePack {

	public class Animations {
		
		Game game;
		IGraphicsApi api;
		Bitmap bmp;
		FastBitmap fastBmp;
		List<AnimationData> animations = new List<AnimationData>();
		
		public Animations( Game game ) {
			this.game = game;
			api = game.Graphics;
		}
		
		public void SetAtlas( Bitmap bmp ) {
			Dispose();
			this.bmp = bmp;
			fastBmp = new FastBitmap( bmp, true );
		}
		
		public void Dispose() {
			if( bmp != null ) {
				fastBmp.Dispose();
				bmp.Dispose();
				bmp = null;
			}
			animations.Clear();
		}
		
		public void Tick( double delta ) {
			if( animations.Count == 0 ) return;
			
			foreach( AnimationData anim in animations ) {
				ApplyAnimation( anim );
			}
		}
		
		public void ReadAnimationsDescription( StreamReader reader ) {
			string line;
			while( ( line = reader.ReadLine() ) != null ) {
				if( line.Length == 0 || line[0] == '#' ) continue;
				
				string[] parts = line.Split( ' ' );
				if( parts.Length < 7 ) {
					Utils.LogWarning( "Not enough arguments for animation: " + line ); continue;
				}
				byte tileX, tileY;
				if( !Byte.TryParse( parts[0], out tileX ) || !Byte.TryParse( parts[1], out tileY ) 
				  || tileX >= 16 || tileY >= 16 ) {
					Utils.LogWarning( "Invalid animation tile coordinates: " + line ); continue;
				}
				
				int frameX, frameY;
				if( !Int32.TryParse( parts[2], out frameX ) || !Int32.TryParse( parts[3], out frameY ) 
				  || frameX < 0 || frameY < 0 ) {
					Utils.LogWarning( "Invalid animation coordinates: " + line ); continue;
				}
				
				int frameSize, statesCount, tickDelay;
				if( !Int32.TryParse( parts[4], out frameSize ) || !Int32.TryParse( parts[5], out statesCount ) ||
				   !Int32.TryParse( parts[6], out tickDelay ) || frameSize < 0 || statesCount < 0 || tickDelay < 0 ) {
					Utils.LogWarning( "Invalid animation: " + line ); continue;
				}
				
				DefineAnimation( tileX, tileY, frameX, frameY, frameSize, statesCount, tickDelay );
			}
		}
		
		public void DefineAnimation( int tileX, int tileY, int frameX, int frameY, int frameSize,
		                            int statesNum, int tickDelay ) {
			AnimationData data = new AnimationData();
			data.TileX = tileX; data.TileY = tileY;
			data.FrameX = frameX; data.FrameY = frameY;
			data.FrameSize = frameSize; data.StatesCount = statesNum;
			data.TickDelay = tickDelay;
			animations.Add( data );
		}
		
		unsafe void ApplyAnimation( AnimationData data ) {
			data.Tick--;
			if( data.Tick >= 0 ) return;
			data.CurrentState++;
			data.CurrentState %= data.StatesCount;			
			data.Tick = data.TickDelay;
			
			TerrainAtlas1D atlas = game.TerrainAtlas1D;
			int texId = ( data.TileY << 4 ) | data.TileX;
			int index = atlas.Get1DIndex( texId );
			int rowNum = atlas.Get1DRowId( texId );
			
			int size = data.FrameSize;
			byte* temp = stackalloc byte[size * size * 4];
			FastBitmap part = new FastBitmap( size, size, size * 4, (IntPtr)temp );
			FastBitmap.MovePortion( data.FrameX + data.CurrentState * size, data.FrameY, 0, 0, fastBmp, part, size );
			api.UpdateTexturePart( atlas.TexIds[index], 0, rowNum * game.TerrainAtlas.elementSize, part );
		}	
		
		class AnimationData {
			public int TileX, TileY;
			public int FrameX, FrameY, FrameSize;
			public int CurrentState;
			public int StatesCount;
			public int Tick, TickDelay;
		}
	}
}
