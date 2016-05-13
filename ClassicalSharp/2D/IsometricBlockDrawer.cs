// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.TexturePack;
using OpenTK;

namespace ClassicalSharp {

	public sealed class IsometricBlockDrawer {
		
		Game game;
		TerrainAtlas1D atlas;
		int index;
		float scale;
		Vector3 minBB, maxBB;
		const float invElemSize = TerrainAtlas2D.invElementSize;
		bool fullBright;
		VertexP3fT2fC4b[] vertices;
		int vb;
		
		public void BeginBatch( Game game, VertexP3fT2fC4b[] vertices, int vb ) {
			this.game = game;
			lastIndex = -1;
			index = 0;
			this.vertices = vertices;
			this.vb = vb;
		}
		
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
		
		public void DrawBatch( byte block, float size, float x, float y ) {
			BlockInfo info = game.BlockInfo;
			atlas = game.TerrainAtlas1D;
			minBB = info.MinBB[block];
			maxBB = info.MaxBB[block];
			fullBright = info.FullBright[block];
			if( info.IsSprite[block] ) { minBB = Vector3.Zero; maxBB = Vector3.One; }
			if( info.IsAir[block] ) return;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;
			Utils.RotateX( ref pos.Y, ref pos.Z, cosX, -sinX );
			Utils.RotateY( ref pos.X, ref pos.Z, cosY, -sinY );
			
			if( info.IsSprite[block] ) {
				SpriteXQuad( block, Side.Right, false );
				SpriteZQuad( block, Side.Back, false );
				
				SpriteZQuad( block, Side.Back, true );
				SpriteXQuad( block, Side.Right, true );
			} else {
				XQuad( block, Make( maxBB.X ), Side.Left );
				ZQuad( block, Make( minBB.Z ), Side.Back );
				YQuad( block, Make( maxBB.Y ), Side.Top );
			}
		}
		
		public void EndBatch() {
			if( index == 0 ) return;
			if( texIndex != lastIndex )
				game.Graphics.BindTexture( atlas.TexIds[texIndex] );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles,
			                                     vb, vertices, index, index * 6 / 4 );
			index = 0;
			lastIndex = -1;
		}

		static Vector3 pos = Vector3.Zero;
		void YQuad( byte block, float y, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
		
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			FastColour col = colNormal;
			v.A = col.A; v.R = col.R; v.G = col.G; v.B = col.B;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + minBB.Z * atlas.invElementSize;
			rec.V2 = vOrigin + maxBB.Z * atlas.invElementSize * (15.99f/16f);

			v.X = Make( minBB.X ); v.Y = y; v.Z = Make( minBB.Z );
			v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = Make( maxBB.X ); v.Y = y; v.Z = Make( minBB.Z );
			v.U = rec.U1; v.V = rec.V2; Transform( ref v );
			v.X = Make( maxBB.X ); v.Y = y; v.Z = Make( maxBB.Z );
			v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = Make( minBB.X ); v.Y = y; v.Z = Make( maxBB.Z );
			v.U = rec.U2; v.V = rec.V1; Transform( ref v );
		}

		void ZQuad( byte block, float z, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			FastColour col = fullBright ? colNormal : colZSide;
			v.A = col.A; v.R = col.R; v.G = col.G; v.B = col.B;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);
			
			v.X = Make( minBB.X ); v.Y = Make( maxBB.Y ); v.Z = z;
			v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = Make( minBB.X ); v.Y = Make( minBB.Y ); v.Z = z;
			v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = Make( maxBB.X ); v.Y = Make( minBB.Y ); v.Z = z;
			v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = Make( maxBB.X ); v.Y = Make( maxBB.Y ); v.Z = z;
			v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}

		void XQuad( byte block, float x, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			FastColour col = fullBright ? colNormal : colXSide;
			v.A = col.A; v.R = col.R; v.G = col.G; v.B = col.B;
			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.Z; rec.U2 = maxBB.Z;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);
			
			v.X = x; v.Y = Make( maxBB.Y ); v.Z = Make( minBB.Z );
			v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = x; v.Y = Make( minBB.Y ); v.Z = Make( minBB.Z );
			v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = x; v.Y = Make( minBB.Y ); v.Z = Make( maxBB.Z );
			v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = x; v.Y = Make( maxBB.Y ); v.Z = Make( maxBB.Z );
			v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}
		
		void SpriteZQuad( byte block, int side, bool firstPart ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			FastColour col = colNormal;
			v.A = col.A; v.R = col.R; v.G = col.G; v.B = col.B;	
			
			float x1 = firstPart ? -0.1f : 0.5f, x2 = firstPart ? 0.5f : 1.1f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			
			v.X = Make( x1 ); v.Y = Make( 0.0f ); v.Z = Make( 0.5f ); 
			v.U = rec.U1; v.V = rec.V2; Transform( ref v );
			v.X = Make( x1 ); v.Y = Make( 1.1f ); v.Z = Make( 0.5f ); 
			v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = Make( x2 ); v.Y = Make( 1.1f ); v.Z = Make( 0.5f ); 
			v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = Make( x2 ); v.Y = Make( 0.0f ); v.Z = Make( 0.5f ); 
			v.U = rec.U2; v.V = rec.V2; Transform( ref v );
		}

		void SpriteXQuad( byte block, int side, bool firstPart ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			FastColour col = colNormal;
			v.A = col.A; v.R = col.R; v.G = col.G; v.B = col.B;
			
			float z1 = firstPart ? -0.1f : 0.5f, z2 = firstPart ? 0.5f : 1.1f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			
			v.X = Make( 0.5f ); v.Y = Make( 0.0f ); v.Z = Make( z1 ); 
			v.U = rec.U1; v.V = rec.V2; Transform( ref v );
			v.X = Make( 0.5f ); v.Y = Make( 1.1f ); v.Z = Make( z1 ); 
			v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = Make( 0.5f ); v.Y = Make( 1.1f ); v.Z = Make( z2 ); 
			v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = Make( 0.5f ); v.Y = Make( 0.0f ); v.Z = Make( z2 ); 
			v.U = rec.U2; v.V = rec.V2; Transform( ref v );
		}
		
		float Make( float value ) { return scale * (1 - value * 2); }
		
		int lastIndex, texIndex;
		void Flush() {
			if( lastIndex != -1 ) {
				game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles,
				                                      vb, vertices, index, index * 6 / 4 );
				index = 0;
			}
			lastIndex = texIndex;
			game.Graphics.BindTexture( atlas.TexIds[texIndex] );
		}
		
		void Transform( ref VertexP3fT2fC4b v ) {
			v.X += pos.X; v.Y += pos.Y; v.Z += pos.Z;
			//Vector3 p = new Vector3( v.X, v.Y, v.Z ) + pos;
			//p = Utils.RotateY( p - pos, time ) + pos;
			//v coords = p
			
			// See comment in IGraphicsApi.Draw2DTexture()
			v.X -= 0.5f; v.Y -= 0.5f;
			float t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t; // Inlined RotY
			t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t;      // Inlined RotX
			vertices[index++] = v;
		}
	}
}