using System;
using System.Collections.Generic;
using System.Drawing;
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
			//SetAnimationAtlas( new Bitmap( "animation.png" ) );
			//DefineAnimation( 0, 0, 0, 0, 16, 7, 5 );
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
			}
		}
		
		public void Tick( double delta ) {
			if( animations.Count == 0 ) return;
			
			foreach( AnimationData anim in animations ) {
				ApplyAnimation( anim );
			}
		}
		
		public void DefineAnimation( int tileX, int tileY, int animX, int animY, int animSize,
		                            int statesNum, int tickDelay ) {
			AnimationData data = new AnimationData();
			data.TileX = tileX; data.TileY = tileY;
			data.AnimX = animX; data.AnimY = animY;
			data.AnimSize = animSize; data.StatesCount = statesNum;
			data.TickDelay = tickDelay;
			animations.Add( data );
		}
		
		unsafe void ApplyAnimation( AnimationData data ) {
			data.Tick--;
			if( data.Tick >= 0 ) return;
			Console.WriteLine( "tick" );
			data.CurrentState++;
			data.CurrentState %= data.StatesCount;			
			data.Tick = data.TickDelay;
			
			TerrainAtlas1D atlas = game.TerrainAtlas1D;
			int texId = ( data.TileY << 4 ) | data.TileX;
			int index = atlas.Get1DIndex( texId );
			int rowNum = atlas.Get1DRowId( texId );
			
			int size = data.AnimSize;
			byte* temp = stackalloc byte[size * size * 4];
			FastBitmap part = new FastBitmap( size, size, size * 4, (IntPtr)temp );
			FastBitmap.MovePortion( data.AnimX + data.CurrentState * size, data.AnimY, 0, 0, fastBmp, part, size );
			api.UpdateTexturePart( atlas.TexIds[index], 0, rowNum * game.TerrainAtlas.elementSize, part );
		}	
		
		class AnimationData {
			public int TileX, TileY;
			public int AnimX, AnimY, AnimSize;
			public int CurrentState;
			public int StatesCount;
			public int Tick, TickDelay;
		}
	}
}
