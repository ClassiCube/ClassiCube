using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Model {

	/// <summary> Contains a set of quads and/or boxes that describe a 3D object as well as
	/// the bounding boxes that contain the entire set of quads and/or boxes. </summary>
	public abstract class IModel : IDisposable {
		
		protected Game game;
		protected ModelCache cache;
		protected IGraphicsApi graphics;
		protected const int quadVertices = 4;
		protected const int boxVertices = 6 * quadVertices;

		public IModel( Game game ) {
			this.game = game;
			graphics = game.Graphics;
			cache = game.ModelCache;
		}
		
		/// <summary> Whether the entity should be slightly bobbed up and down when rendering. </summary>
		/// <remarks> e.g. for players when their legs are at the peak of their swing,
		/// the whole model will be moved slightly down. </remarks>
		public abstract bool Bobbing { get; }
		
		/// <summary> Vertical offset from the model's feet/base that the name texture should be drawn at. </summary>
		public abstract float NameYOffset { get; }
		
		/// <summary> Vertical offset from the model's feet/base that the model's eye is located. </summary>
		public abstract float GetEyeY( Player player );
		
		/// <summary> The size of the bounding box that is used when
		/// performing collision detection for this model. </summary>
		public abstract Vector3 CollisionSize { get; }
		
		/// <summary> Bounding box that contains this model,
		/// assuming that the model is not rotated at all.</summary>
		public abstract BoundingBox PickingBounds { get; }
		
		protected Vector3 pos;
		protected float cosA, sinA;
		
		/// <summary> Renders the model based on the given entity's position and orientation. </summary>
		public void RenderModel( Player p ) {
			index = 0;
			pos = p.Position;
			if( Bobbing )
				pos.Y += p.bobYOffset;
			cosA = (float)Math.Cos( p.YawRadians );
			sinA = (float)Math.Sin( p.YawRadians );
			Map map = game.Map;
			col = game.Map.IsLit( Vector3I.Floor( p.EyePosition ) ) ? map.Sunlight : map.Shadowlight;

			graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			DrawPlayerModel( p );
		}
		
		protected abstract void DrawPlayerModel( Player p );
		
		public virtual void Dispose() {
		}
		
		protected FastColour col;
		protected ModelVertex[] vertices;
		protected int index;
		
		public struct BoxDescription {
			public int TexX, TexY, SidesW, BodyW, BodyH;
			public float X1, X2, Y1, Y2, Z1, Z2;
			
			public BoxDescription SetTexOrigin( int x, int y ) {
				TexX = x; TexY = y; return this;
			}
			
			public BoxDescription SetModelBounds( float x1, float y1, float z1, float x2, float y2, float z2 ) {
				X1 = x1 / 16f; X2 = x2 / 16f;
				Y1 = y1 / 16f; Y2 = y2 / 16f;
				Z1 = z1 / 16f; Z2 = z2 / 16f;
				return this;
			}
		}
		
		protected BoxDescription MakeBoxBounds( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			BoxDescription desc = default( BoxDescription )
				.SetModelBounds( x1, y1, z1, x2, y2, z2 );
			desc.SidesW = Math.Abs( z2 - z1 );
			desc.BodyW = Math.Abs( x2 - x1 );
			desc.BodyH = Math.Abs( y2 - y1 );
			return desc;
		}
		
		protected BoxDescription MakeRotatedBoxBounds( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			BoxDescription desc = default( BoxDescription )
				.SetModelBounds( x1, y1, z1, x2, y2, z2 );
			desc.SidesW = Math.Abs( y2 - y1 );
			desc.BodyW = Math.Abs( x2 - x1 );
			desc.BodyH = Math.Abs( z2 - z1 );
			return desc;
		}
		
		/// <summary>Builds a box model assuming the follow texture layout:<br/>
		/// let SW = sides width, BW = body width, BH = body height<br/>
		/// ┏━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━┓ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃S┈┈┈┈┈┈┈┈┈┈┈S┃S┈┈┈┈top┈┈┈┈S┃S┈┈bottom┈┈┈S┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃W┈┈┈┈┈┈┈┈┈W┃W┈┈┈┈tex┈┈┈W┃W┈┈┈┈tex┈┈┈┈W┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━╋━━━━━━━━━━━━━━╋━━━━━━━━━━━━┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┃B┈┈┈left┈┈┈┈┈B┃B┈┈front┈┈┈┈B┃B┈┈┈right┈┈┈┈B┃B┈┈┈back┈┈┈B┃ <br/>
		/// ┃H┈┈┈tex┈┈┈┈┈H┃H┈┈tex┈┈┈┈┈H┃H┈┈┈tex┈┈┈┈┈H┃H┈┈┈┈tex┈┈┈H┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┗━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┛ </summary>
		protected ModelPart BuildBox( BoxDescription desc ) {
			int sidesW = desc.SidesW, bodyW = desc.BodyW, bodyH = desc.BodyH;
			float x1 = desc.X1, y1 = desc.Y1, z1 = desc.Z1;
			float x2 = desc.X2, y2 = desc.Y2, z2 = desc.Z2;
			int x = desc.TexX, y = desc.TexY;
			
			YQuad( x + sidesW, y, bodyW, sidesW, x2, x1, z2, z1, y2 ); // top
			YQuad( x + sidesW + bodyW, y, bodyW, sidesW, x2, x1, z1, z2, y1 ); // bottom
			ZQuad( x + sidesW, y + sidesW, bodyW, bodyH, x2, x1, y1, y2, z1 ); // front
			ZQuad( x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x1, x2, y1, y2, z2 ); // back
			XQuad( x, y + sidesW, sidesW, bodyH, z2, z1, y1, y2, x2 ); // left
			XQuad( x + sidesW + bodyW, y + sidesW, sidesW, bodyH, z1, z2, y1, y2, x1 ); // right
			return new ModelPart( index - 6 * 4, 6 * 4 );
		}
		
		/// <summary>Builds a box model assuming the follow texture layout:<br/>
		/// let SW = sides width, BW = body width, BH = body height<br/>
		/// ┏━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━┓ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃S┈┈┈┈┈┈┈┈┈┈┈S┃S┈┈┈┈front┈┈┈S┃S┈┈┈back┈┈┈┈S┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃W┈┈┈┈┈┈┈┈┈W┃W┈┈┈┈tex┈┈┈W┃W┈┈┈┈tex┈┈┈┈W┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━╋━━━━━━━━━━━━━━╋━━━━━━━━━━━━┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┃B┈┈┈left┈┈┈┈┈B┃B┈┈bottom┈┈B┃B┈┈┈right┈┈┈┈B┃B┈┈┈┈top┈┈┈B┃ <br/>
		/// ┃H┈┈┈tex┈┈┈┈┈H┃H┈┈tex┈┈┈┈┈H┃H┈┈┈tex┈┈┈┈┈H┃H┈┈┈┈tex┈┈┈H┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┗━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┛ </summary>
		protected ModelPart BuildRotatedBox( BoxDescription desc ) {
			int sidesW = desc.SidesW, bodyW = desc.BodyW, bodyH = desc.BodyH;
			float x1 = desc.X1, y1 = desc.Y1, z1 = desc.Z1;
			float x2 = desc.X2, y2 = desc.Y2, z2 = desc.Z2;
			int x = desc.TexX, y = desc.TexY;
			
			YQuad( x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x1, x2, z1, z2, y2 ); // top
			YQuad( x + sidesW, y + sidesW, bodyW, bodyH, x2, x1, z1, z2, y1 ); // bottom
			ZQuad( x + sidesW, y, bodyW, sidesW, x2, x1, y1, y2, z1 ); // front
			ZQuad( x + sidesW + bodyW, y, bodyW, sidesW, x1, x2, y2, y1, z2 ); // back
			XQuad( x, y + sidesW, sidesW, bodyH, y2, y1, z2, z1, x2 ); // left
			XQuad( x + sidesW + bodyW, y + sidesW, sidesW, bodyH, y1, y2, z2, z1, x1 ); // right
			
			// rotate left and right 90 degrees
			for( int i = index - 8; i < index; i++ ) {
				ModelVertex vertex = vertices[i];
				float z = vertex.Z; vertex.Z = vertex.Y; vertex.Y = z;
				vertices[i] = vertex;
			}
			return new ModelPart( index - 6 * 4, 6 * 4 );
		}
		
		protected void XQuad( int texX, int texY, int texWidth, int texHeight,
		                     float z1, float z2, float y1, float y2, float x ) {
			vertices[index++] = new ModelVertex( x, y1, z1, texX, texY + texHeight );
			vertices[index++] = new ModelVertex( x, y2, z1, texX, texY );
			vertices[index++] = new ModelVertex( x, y2, z2, texX + texWidth, texY );
			vertices[index++] = new ModelVertex( x, y1, z2, texX + texWidth, texY + texHeight );
		}
		
		protected void YQuad( int texX, int texY, int texWidth, int texHeight,
		                     float x1, float x2, float z1, float z2, float y ) {
			vertices[index++] = new ModelVertex( x1, y, z2, texX, texY + texHeight );
			vertices[index++] = new ModelVertex( x1, y, z1, texX, texY );
			vertices[index++] = new ModelVertex( x2, y, z1, texX + texWidth, texY );
			vertices[index++] = new ModelVertex( x2, y, z2, texX + texWidth, texY + texHeight );
		}
		
		protected void ZQuad( int texX, int texY, int texWidth, int texHeight,
		                     float x1, float x2, float y1, float y2, float z ) {
			vertices[index++] = new ModelVertex( x1, y1, z, texX, texY + texHeight );
			vertices[index++] = new ModelVertex( x1, y2, z, texX, texY );
			vertices[index++] = new ModelVertex( x2, y2, z, texX + texWidth, texY );
			vertices[index++] = new ModelVertex( x2, y1, z, texX + texWidth, texY + texHeight );
		}
		
		protected bool _64x64 = false;
		protected void DrawPart( ModelPart part ) {
			float vScale = _64x64 ? 64f : 32f;
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex model = vertices[part.Offset + i];
				Vector3 newPos = Utils.RotateY( model.X, model.Y, model.Z, cosA, sinA ) + pos;
				
				FastColour col = GetCol( i, part.Count );
				VertexPos3fTex2fCol4b vertex = default( VertexPos3fTex2fCol4b );
				vertex.X = newPos.X; vertex.Y = newPos.Y; vertex.Z = newPos.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				AdjustUV( model.U, model.V, vScale, i, ref vertex );
				cache.vertices[index++] = vertex;
			}
		}
		
		protected void DrawRotate( float x, float y, float z, float angleX, float angleY, float angleZ, ModelPart part ) {
			float cosX = (float)Math.Cos( -angleX ), sinX = (float)Math.Sin( -angleX );
			float cosY = (float)Math.Cos( -angleY ), sinY = (float)Math.Sin( -angleY );
			float cosZ = (float)Math.Cos( -angleZ ), sinZ = (float)Math.Sin( -angleZ );
			float vScale = _64x64 ? 64f : 32f;
			
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex model = vertices[part.Offset + i];
				Vector3 loc = new Vector3( model.X - x, model.Y - y, model.Z - z );
				loc = Utils.RotateZ( loc.X, loc.Y, loc.Z, cosZ, sinZ );
				loc = Utils.RotateY( loc.X, loc.Y, loc.Z, cosY, sinY );
				loc = Utils.RotateX( loc.X, loc.Y, loc.Z, cosX, sinX );
				
				FastColour col = GetCol( i, part.Count );				
				VertexPos3fTex2fCol4b vertex = default( VertexPos3fTex2fCol4b );
				Vector3 newPos = Utils.RotateY( loc.X + x, loc.Y + y, loc.Z + z, cosA, sinA ) + pos;
				vertex.X = newPos.X; vertex.Y = newPos.Y; vertex.Z = newPos.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				AdjustUV( model.U, model.V, vScale, i, ref vertex );				
				cache.vertices[index++] = vertex;
			}
		}
		
		FastColour GetCol( int i, int count ) {
			if( count != boxVertices )
				return FastColour.Scale( col, 0.7f );
			if( i >= 4 && i < 8 )
				return FastColour.Scale( col, FastColour.ShadeYBottom );
			if( i >= 8 && i < 16 ) 
				return FastColour.Scale( col, FastColour.ShadeZ );
			if( i >= 16 && i < 24 ) 
				return FastColour.Scale( col, FastColour.ShadeX );
			return col;
		}
		
		void AdjustUV( ushort u, ushort v, float vScale, int i, ref VertexPos3fTex2fCol4b vertex ) {
			vertex.U = u / 64f; vertex.V = v / vScale;
			int quadIndex = i % 4;
			if( quadIndex == 0 || quadIndex == 3 )
				vertex.V -= 0.01f / vScale;
			if( quadIndex == 2 || quadIndex == 3 )
				vertex.U -= 0.01f / 64f;
		}
		
		[StructLayout( LayoutKind.Sequential, Pack = 1 )]
		protected struct ModelVertex {
			public float X, Y, Z;
			public ushort U, V;
			
			public ModelVertex( float x, float y, float z, int u, int v ) {
				X = x; Y = y; Z = z;
				U = (ushort)u; V = (ushort)v;
			}
		}
	}
}