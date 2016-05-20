// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp.Singleplayer {

	public class TNTPhysics {
		Game game;
		World map;
		Random rnd = new Random();
		
		public TNTPhysics( Game game, Physics physics ) {
			this.game = game;
			map = game.World;
			physics.OnPlace[(byte)Block.TNT] = HandleTnt;
		}
		
		Vector3[] rayDirs;
		const float stepLen = 0.3f;
		float[] hardness;
		
		void HandleTnt( int index, byte block ) {
			int x = index % map.Width;
			int z = (index / map.Width) % map.Length;
			int y = (index / map.Width) / map.Length;
			Explode( 4, x, y, z );
		}
		
		// Algorithm source: http://minecraft.gamepedia.com/Explosion
		public void Explode( float power, int x, int y, int z ) {
			if( rayDirs == null )
				InitExplosionCache();
			
			game.UpdateBlock( x, y, z, 0 );
			Vector3 basePos = new Vector3( x, y, z );
			for( int i = 0; i < rayDirs.Length; i++ ) {
				Vector3 dir = rayDirs[i] * stepLen;
				Vector3 position = basePos;
				float intensity = (float)(0.7 + rnd.NextDouble() * 0.6) * power;
				while( intensity > 0 ) {
					position += dir;
					intensity -= stepLen * 0.75f;
					Vector3I blockPos = Vector3I.Floor( position );
					if( !map.IsValidPos( blockPos ) ) break;
					
					byte block = map.GetBlock( blockPos );
					intensity -= (hardness[block] / 5 + 0.3f) * stepLen;
					if( intensity > 0 && block != 0 ) {
						game.UpdateBlock( blockPos.X, blockPos.Y, blockPos.Z, 0 );
					}
				}
			}
		}
		
		void InitExplosionCache() {
			hardness = new float[] { 0, 30, 3, 2.5f, 30, 15, 0, 1.8E+07f, 500, 500, 500, 500, 2.5f,
				3, 15, 15, 15, 10, 1, 3, 1.5f, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
				0, 0, 30, 30, 30, 30, 30, 0, 7.5f, 30, 6000, 30, 0, 4, 0.5f, 0, 4, 4, 4, 4, 4, 2.5f,
				// Note that the 30, 500, 15, 15 are guesses (CeramicTile --> Crate)
				30, 500, 15, 15, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			rayDirs = new Vector3[1352];
			int index = 0;
			
			// Y bottom and top planes
			for( float x = -1; x <= 1.001f; x += 2f/15) {
				for( float z = -1; z <= 1.001f; z += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( x, -1, z );
					rayDirs[index++] = Vector3.Normalize( x, +1, z );
				}
			}
			// Z planes
			for( float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for( float x = -1; x <= 1.001f; x += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( x, y, -1 );
					rayDirs[index++] = Vector3.Normalize( x, y, +1 );
				}
			}
			// X planes
			for( float y = -1 + 2f/15; y <= 1.001f - 2f/15; y += 2f/15) {
				for( float z = -1 + 2f/15; z <= 1.001f- 2f/15; z += 2f/15) {
					rayDirs[index++] = Vector3.Normalize( -1, y, z );
					rayDirs[index++] = Vector3.Normalize( +1, y, z );
				}
			}
		}
	}
}