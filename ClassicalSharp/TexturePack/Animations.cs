// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.TexturePack {

	/// <summary> Contains and describes the various animations applied to the terrain atlas. </summary>
	public class Animations {
		
		Game game;
		IGraphicsApi api;
		Bitmap bmp;
		FastBitmap fastBmp;
		List<AnimationData> animations = new List<AnimationData>();
		public bool Enabled = true;
		
		public Animations( Game game ) {
			this.game = game;
			api = game.Graphics;
		}
		
		/// <summary> Sets the atlas bitmap that animation frames are contained within. </summary>
		public void SetAtlas( Bitmap bmp ) {
			Dispose();
			this.bmp = bmp;
			fastBmp = new FastBitmap( bmp, true, true );
		}
		
		/// <summary> Runs through all animations and if necessary updates the terrain atlas. </summary>
		public void Tick( double delta ) {
			if( animations.Count == 0 || !Enabled ) return;
			
			foreach( AnimationData anim in animations ) {
				ApplyAnimation( anim );
			}
		}
		
		/// <summary> Reads a text file that contains a number of lines, with each line describing:<br/>
		/// 1) the target tile in the terrain atlas  2) the start location of animation frames<br/>
		/// 3) the size of each animation frame      4) the number of animation frames<br/>
		/// 5) the delay between advancing animation frames. </summary>
		public void ReadAnimationsDescription( StreamReader reader ) {
			string line;
			while( (line = reader.ReadLine()) != null ) {
				if( line.Length == 0 || line[0] == '#' ) continue;
				
				string[] parts = line.Split( ' ' );
				if( parts.Length < 7 ) {
					Utils.LogDebug( "Not enough arguments for animation: {0}", line ); continue;
				}
				
				byte tileX, tileY;
				if( !Byte.TryParse( parts[0], out tileX ) || !Byte.TryParse( parts[1], out tileY )
				   || tileX >= 16 || tileY >= 16 ) {
					Utils.LogDebug( "Invalid animation tile coordinates: {0}", line ); continue;
				}
				
				int frameX, frameY;
				if( !Int32.TryParse( parts[2], out frameX ) || !Int32.TryParse( parts[3], out frameY )
				   || frameX < 0 || frameY < 0 ) {
					Utils.LogDebug( "Invalid animation coordinates: {0}", line ); continue;
				}
				
				int frameSize, statesCount, tickDelay;
				if( !Int32.TryParse( parts[4], out frameSize ) || !Int32.TryParse( parts[5], out statesCount ) ||
				   !Int32.TryParse( parts[6], out tickDelay ) || frameSize < 0 || statesCount < 0 || tickDelay < 0 ) {
					Utils.LogDebug( "Invalid animation: {0}", line ); continue;
				}
				
				DefineAnimation( tileX, tileY, frameX, frameY, frameSize, statesCount, tickDelay );
			}
		}
		
		/// <summary> Adds an animation described by the arguments to the list of animations
		/// that are applied to the terrain atlas. </summary>
		public void DefineAnimation( int tileX, int tileY, int frameX, int frameY, int frameSize,
		                            int statesNum, int tickDelay ) {
			AnimationData data = new AnimationData();
			data.TileX = tileX; data.TileY = tileY;
			data.FrameX = frameX; data.FrameY = frameY;
			data.FrameSize = frameSize; data.StatesCount = statesNum;
			data.TickDelay = tickDelay;
			animations.Add( data );
		}
		
		FastBitmap animPart = new FastBitmap();
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
			animPart.SetData( size, size, size * 4, (IntPtr)temp, false );
			FastBitmap.MovePortion( data.FrameX + data.CurrentState * size, data.FrameY, 0, 0, fastBmp, animPart, size );
			api.UpdateTexturePart( atlas.TexIds[index], 0, rowNum * game.TerrainAtlas.elementSize, animPart );
		}
		
		class AnimationData {
			public int TileX, TileY; // Tile (not pixel) coordinates in terrain.png
			public int FrameX, FrameY; // Top left pixel coordinates of start frame in animatons.png
			public int FrameSize; // Size of each frame in pixel coordinates
			public int CurrentState; // Current animation frame index
			public int StatesCount; // Total number of animation frames
			public int Tick, TickDelay;
		}
		
		/// <summary> Disposes the atlas bitmap that contains animation frames, and clears
		/// the list of defined animations. </summary>
		public void Dispose() {
			animations.Clear();
			
			if( bmp == null ) return;
			fastBmp.Dispose();
			bmp.Dispose();
			bmp = null;
		}
	}
}
