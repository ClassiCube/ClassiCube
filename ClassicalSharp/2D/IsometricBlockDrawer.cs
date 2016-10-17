// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Textures;
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
		
		static int colNormal = FastColour.WhitePacked, colXSide, colZSide, colYBottom;
		static float cosX, sinX, cosY, sinY;
		static IsometricBlockDrawer() {
			FastColour.GetShaded( FastColour.White, out colXSide, out colZSide, out colYBottom );
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
				SpriteXQuad( block, true );
				SpriteZQuad( block, true );				
				
				SpriteZQuad( block, false );
				SpriteXQuad( block, false );				
			} else {
				XQuad( block, maxBB.X, Side.Left );
				ZQuad( block, minBB.Z, Side.Back );
				YQuad( block, maxBB.Y, Side.Top );
			}
		}
		
		public void EndBatch() {
			if( index == 0 ) return;
			if( texIndex != lastIndex )
				game.Graphics.BindTexture( atlas.TexIds[texIndex] );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles,
			                                     vb, vertices, index );
			index = 0;
			lastIndex = -1;
		}

		static Vector3 pos = Vector3.Zero;
		void YQuad( byte block, float y, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			texIndex = texLoc / atlas.elementsPerAtlas1D;
			if( lastIndex != texIndex ) Flush();
		
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			TextureRec rec;
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + minBB.Z * atlas.invElementSize;
			rec.V2 = vOrigin + maxBB.Z * atlas.invElementSize * (15.99f/16f);

			y = scale * (1 - y * 2);			
			float minX = scale * (1 - minBB.X * 2), maxX = scale * (1 - maxBB.X * 2);
			float minZ = scale * (1 - minBB.Z * 2), maxZ = scale * (1 - maxBB.Z * 2);
			v.X = minX; v.Y = y; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = maxX; v.Y = y; v.Z = minZ; v.U = rec.U1; v.V = rec.V2; Transform( ref v );
			v.X = maxX; v.Y = y; v.Z = maxZ; v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = minX; v.Y = y; v.Z = maxZ; v.U = rec.U2; v.V = rec.V1; Transform( ref v );
		}

		void ZQuad( byte block, float z, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			texIndex = texLoc / atlas.elementsPerAtlas1D;
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = fullBright ? colNormal : colZSide;

			TextureRec rec;			
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.X; rec.U2 = maxBB.X;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);

			z = scale * (1 - z * 2);			
			float minX = scale * (1 - minBB.X * 2), maxX = scale * (1 - maxBB.X * 2);
			float minY = scale * (1 - minBB.Y * 2), maxY = scale * (1 - maxBB.Y * 2);			
			v.X = minX; v.Y = maxY; v.Z = z; v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = minX; v.Y = minY; v.Z = z; v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = maxX; v.Y = minY; v.Z = z; v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = maxX; v.Y = maxY; v.Z = z; v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}

		void XQuad( byte block, float x, int side ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, side );
			texIndex = texLoc / atlas.elementsPerAtlas1D;
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = fullBright ? colNormal : colXSide;
			
			TextureRec rec;
			float vOrigin = (texLoc % atlas.elementsPerAtlas1D) * atlas.invElementSize;
			rec.U1 = minBB.Z; rec.U2 = maxBB.Z;
			rec.V1 = vOrigin + (1 - minBB.Y) * atlas.invElementSize;
			rec.V2 = vOrigin + (1 - maxBB.Y) * atlas.invElementSize * (15.99f/16f);
			
			x = scale * (1 - x * 2);
			float minY = scale * (1 - minBB.Y * 2), maxY = scale * (1 - maxBB.Y * 2);
			float minZ = scale * (1 - minBB.Z * 2), maxZ = scale * (1 - maxBB.Z * 2);	
			v.X = x; v.Y = maxY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = x; v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = x; v.Y = minY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = x; v.Y = maxY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}
		
		void SpriteZQuad( byte block, bool firstPart ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, Side.Right );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			float x1 = firstPart ? 0.5f : -0.1f, x2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minX = scale * (1 - x1 * 2), maxX = scale * (1 - x2 * 2);
			float minY = scale * (1 - 0 * 2), maxY = scale * (1 - 1.1f * 2);			
			
			v.X = minX; v.Y = minY; v.Z = 0; v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = minX; v.Y = maxY; v.Z = 0; v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = maxX; v.Y = maxY; v.Z = 0; v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = maxX; v.Y = minY; v.Z = 0; v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}

		void SpriteXQuad( byte block, bool firstPart ) {
			int texLoc = game.BlockInfo.GetTextureLoc( block, Side.Right );
			TextureRec rec = atlas.GetTexRec( texLoc, 1, out texIndex );
			if( lastIndex != texIndex ) Flush();
			
			VertexP3fT2fC4b v = default(VertexP3fT2fC4b);
			v.Colour = colNormal;
			
			float z1 = firstPart ? 0.5f : -0.1f, z2 = firstPart ? 1.1f : 0.5f;
			rec.U1 = firstPart ? 0.0f : 0.5f; rec.U2 = (firstPart ? 0.5f : 1.0f) * (15.99f/16f);
			float minY = scale * (1 - 0 * 2), maxY = scale * (1 - 1.1f * 2);
			float minZ = scale * (1 - z1 * 2), maxZ = scale * (1 - z2 * 2);
			
			v.X = 0; v.Y = minY; v.Z = minZ; v.U = rec.U2; v.V = rec.V2; Transform( ref v );
			v.X = 0; v.Y = maxY; v.Z = minZ; v.U = rec.U2; v.V = rec.V1; Transform( ref v );
			v.X = 0; v.Y = maxY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V1; Transform( ref v );
			v.X = 0; v.Y = minY; v.Z = maxZ; v.U = rec.U1; v.V = rec.V2; Transform( ref v );
		}
		
		int lastIndex, texIndex;
		void Flush() {
			if( lastIndex != -1 ) {
				game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices, index );
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