// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.TexturePack;
using OpenTK;

namespace ClassicalSharp {

	public static class IsometricBlockDrawer {
		
		static BlockInfo info;
		static ModelCache cache;
		static IGraphicsApi api;
		static TerrainAtlas1D atlas;
		static int index;
		static float scale;
		static Vector3 minBB, maxBB;
		const float invElemSize = TerrainAtlas2D.invElementSize;
		static bool fullBright;
		
		static FastColour colNormal, colXSide, colZSide, colYBottom;
		static float cosX, sinX, cosY, sinY;
		static IsometricBlockDrawer() {
			colNormal = FastColour.White;
			FastColour.GetShaded( colNormal, ref colXSide, ref colZSide, ref colYBottom );
			
			cosX = (float)Math.Cos( 26.565f * Utils.Deg2Rad );
			sinX = (float)Math.Sin( 26.565f * Utils.Deg2Rad );
			cosY = (float)Math.Cos( -45f * Utils.Deg2Rad );
			sinY = (float)Math.Sin( -45f * Utils.Deg2Rad );
		}
		
		public static void Draw( Game game, byte block, float size, float x, float y ) {
			info = game.BlockInfo;
			cache = game.ModelCache;
			atlas = game.TerrainAtlas1D;
			minBB = info.MinBB[block];
			maxBB = info.MaxBB[block];
			fullBright = info.FullBright[block];
			if( info.IsSprite[block] ) {
				minBB = Vector3.Zero; maxBB = Vector3.One;
			}
			if( info.IsAir[block] ) return;
			index = 0;
			api = game.Graphics;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;
			Utils.RotateX( ref pos.Y, ref pos.Z, cosX, -sinX );
			Utils.RotateY( ref pos.X, ref pos.Z, cosY, -sinY );
			
			if( info.IsSprite[block] ) {
				SpriteXQuad( block, TileSide.Right, false );
				SpriteZQuad( block, TileSide.Back, false );
				
				SpriteZQuad( block, TileSide.Back, true );
				SpriteXQuad( block, TileSide.Right, true );
			} else {
				XQuad( block, Make( maxBB.X ), TileSide.Left );
				ZQuad( block, Make( minBB.Z ), TileSide.Back );
				YQuad( block, Make( maxBB.Y ), TileSide.Top );
			}
			
			if( index == 0 ) return;
			if( atlas.TexIds[texIndex] != lastTexId ) {
				lastTexId = atlas.TexIds[texIndex];
				api.BindTexture( lastTexId );
			}
			for( int i = 0; i < index; i++ )
				TransformVertex( ref cache.vertices[i] );
			api.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}

		static Vector3 pos = Vector3.Zero;
		static void YQuad( byte block, float y, int side ) {
			int texLoc = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			FlushIfNotSame();
			FastColour col = colNormal;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + minBB.Z * atlas.invElementSize;
			rec.V2 = vOrigin + maxBB.Z * atlas.invElementSize * (15.99f/16f);

			cache.vertices[index++] = new VertexP3fT2fC4b( Make( minBB.X ), y, Make( minBB.Z ), rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( maxBB.X ), y, Make( minBB.Z ), rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( maxBB.X ), y, Make( maxBB.Z ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( minBB.X ), y, Make( maxBB.Z ), rec.U2, rec.V1, col );
		}

		static void ZQuad( byte block, float z, int side ) {
			int texLoc = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			FlushIfNotSame();
			FastColour col = fullBright ? colNormal : colZSide;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);
			
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( minBB.X ), Make( maxBB.Y ), z, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( minBB.X ), Make( minBB.Y ), z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( maxBB.X ), Make( minBB.Y ), z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( maxBB.X ), Make( maxBB.Y ), z, rec.U1, rec.V2, col );
		}

		static void XQuad( byte block, float x, int side ) {
			int texLoc = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			FlushIfNotSame();
			FastColour col = fullBright ? colNormal : colXSide;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.Z; rec.U2 = maxBB.Z;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);
			
			cache.vertices[index++] = new VertexP3fT2fC4b( x, Make( maxBB.Y ), Make( minBB.Z ), rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( x, Make( minBB.Y ), Make( minBB.Z ), rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( x, Make( minBB.Y ), Make( maxBB.Z ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( x, Make( maxBB.Y ), Make( maxBB.Z ), rec.U1, rec.V2, col );
		}
		
		static void SpriteZQuad( byte block, int side, bool firstPart ) {
			int texLoc = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			FlushIfNotSame();
			
			FastColour col = colNormal;
			float x1 = firstPart ? -0.1f : 0.5f, x2 = firstPart ? 0.5f : 1.1f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( x1 ), Make( 0.0f ), Make( 0.5f ), rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( x1 ), Make( 1.1f ), Make( 0.5f ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( x2 ), Make( 1.1f ), Make( 0.5f ), rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( x2 ), Make( 0.0f ), Make( 0.5f ), rec.U2, rec.V2, col );
		}

		static void SpriteXQuad( byte block, int side, bool firstPart ) {
			int texLoc = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			FlushIfNotSame();
			
			FastColour col = colNormal;
			float z1 = firstPart ? -0.1f : 0.5f, z2 = firstPart ? 0.5f : 1.1f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( 0.5f ), Make( 0.0f ), Make( z1 ), rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( 0.5f ), Make( 1.1f ), Make( z1 ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( 0.5f ), Make( 1.1f ), Make( z2 ), rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexP3fT2fC4b( Make( 0.5f ), Make( 0.0f ), Make( z2 ), rec.U2, rec.V2, col );
		}
		
		static float Make( float value ) { return scale - (scale * value * 2); }
		
		internal static int lastTexId, texIndex;
		static void FlushIfNotSame() {
			int texId = atlas.TexIds[texIndex];
			if( texId == lastTexId ) return;
			
			if( lastTexId != -1 ) {	
				for( int i = 0; i < index; i++ )
					TransformVertex( ref cache.vertices[i] );
				api.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb,
				                           cache.vertices, index, index * 6 / 4 );
				index = 0;
			}
			lastTexId = texId;
			api.BindTexture( texId );
		}
		
		static void TransformVertex( ref VertexP3fT2fC4b v ) {
			v.X += pos.X; v.Y += pos.Y; v.Z += pos.Z;
			//Vector3 p = new Vector3( v.X, v.Y, v.Z ) + pos;
			//p = Utils.RotateY( p - pos, time ) + pos;
			//v coords = p
			
			// See comment in IGraphicsApi.Draw2DTexture()
			v.X -= 0.5f; v.Y -= 0.5f;
			float t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t; // Inlined RotY
			t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t;      // Inlined RotX
		}
	}
}