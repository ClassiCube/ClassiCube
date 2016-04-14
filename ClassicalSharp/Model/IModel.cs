// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
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
		protected RotateOrder Rotate = RotateOrder.ZYX;

		public IModel( Game game ) {
			this.game = game;
			graphics = game.Graphics;
			cache = game.ModelCache;
		}
		
		internal abstract void CreateParts();
		
		/// <summary> Whether the entity should be slightly bobbed up and down when rendering. </summary>
		/// <remarks> e.g. for players when their legs are at the peak of their swing,
		/// the whole model will be moved slightly down. </remarks>
		public abstract bool Bobbing { get; }
		
		/// <summary> Vertical offset from the model's feet/base that the name texture should be drawn at. </summary>
		public abstract float NameYOffset { get; }
		
		/// <summary> Vertical offset from the model's feet/base that the model's eye is located. </summary>
		public abstract float GetEyeY( Entity entity );
		
		/// <summary> The size of the bounding box that is used when
		/// performing collision detection for this model. </summary>
		public abstract Vector3 CollisionSize { get; }
		
		/// <summary> Bounding box that contains this model,
		/// assuming that the model is not rotated at all.</summary>
		public abstract BoundingBox PickingBounds { get; }
		
		protected Vector3 pos;
		protected float cosYaw, sinYaw, cosHead, sinHead;
		
		/// <summary> Renders the model based on the given entity's position and orientation. </summary>
		public void Render( Player p ) {
			index = 0;
			pos = p.Position;
			if( Bobbing )
				pos.Y += p.anim.bobYOffset;
			World map = game.World;
			col = game.World.IsLit( Vector3I.Floor( p.EyePosition ) ) ? map.Sunlight : map.Shadowlight;
			
			cols[0] = col;
			cols[1] = FastColour.Scale( col, FastColour.ShadeYBottom );
			cols[2] = FastColour.Scale( col, FastColour.ShadeZ ); cols[3] = cols[2];
			cols[4] = FastColour.Scale( col, FastColour.ShadeX ); cols[5] = cols[4];
			
			cosYaw = (float)Math.Cos( p.YawDegrees * Utils.Deg2Rad );
			sinYaw = (float)Math.Sin( p.YawDegrees * Utils.Deg2Rad );
			cosHead = (float)Math.Cos( p.HeadYawDegrees * Utils.Deg2Rad );
			sinHead = (float)Math.Sin( p.HeadYawDegrees * Utils.Deg2Rad );

			graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			DrawModel( p );
		}
		
		protected abstract void DrawModel( Player p );
		
		public virtual void Dispose() { }
		
		protected FastColour col;
		protected FastColour[] cols = new FastColour[6];
		protected internal ModelVertex[] vertices;
		protected internal int index;
		
		protected BoxDesc MakeBoxBounds( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			return ModelBuilder.MakeBoxBounds( x1, y1, z1, x2, y2, z2 );
		}
		
		protected BoxDesc MakeRotatedBoxBounds( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			return ModelBuilder.MakeRotatedBoxBounds( x1, y1, z1, x2, y2, z2 );
		}
		
		protected ModelPart BuildBox( BoxDesc desc ) {
			return ModelBuilder.BuildBox( this, desc );
		}
		
		protected ModelPart BuildRotatedBox( BoxDesc desc ) {
			return ModelBuilder.BuildRotatedBox( this, desc );
		}
		
		protected bool _64x64 = false;
		protected void DrawPart( ModelPart part ) {
			float vScale = _64x64 ? 64f : 32f;
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex model = vertices[part.Offset + i];
				Utils.RotateY( ref model.X, ref model.Z, cosYaw, sinYaw );
				model.X += pos.X; model.Y += pos.Y; model.Z += pos.Z;
				
				FastColour col = part.Count == boxVertices ? 
					cols[i >> 2] : FastColour.Scale( this.col, 0.7f );
				VertexP3fT2fC4b vertex = default( VertexP3fT2fC4b );
				vertex.X = model.X; vertex.Y = model.Y; vertex.Z = model.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				
				vertex.U = model.U / 64f; vertex.V = model.V / vScale;
				int quadI = i % 4;
				if( quadI == 0 || quadI == 3 ) vertex.V -= 0.01f / vScale;
				if( quadI == 2 || quadI == 3 ) vertex.U -= 0.01f / 64f;
				cache.vertices[index++] = vertex;
			}
		}
		
		protected void DrawRotate( float angleX, float angleY, float angleZ, ModelPart part ) {
			DrawRotated( part.RotX, part.RotY, part.RotZ, angleX, angleY, angleZ, part, false );
		}
		
		protected void DrawHeadRotate( float angleX, float angleY, float angleZ, ModelPart part ) {
			DrawRotated( part.RotX, part.RotY, part.RotZ, angleX, angleY, angleZ, part, true );
		}
		
		protected void DrawRotated( float x, float y, float z, float angleX, float angleY, float angleZ, ModelPart part, bool head ) {
			float cosX = (float)Math.Cos( -angleX ), sinX = (float)Math.Sin( -angleX );
			float cosY = (float)Math.Cos( -angleY ), sinY = (float)Math.Sin( -angleY );
			float cosZ = (float)Math.Cos( -angleZ ), sinZ = (float)Math.Sin( -angleZ );
			float vScale = _64x64 ? 64f : 32f;
			
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex model = vertices[part.Offset + i];
				model.X -= x; model.Y -= y; model.Z -= z;
				
				// Rotate locally
				if( Rotate == RotateOrder.ZYX ) {
					Utils.RotateZ( ref model.X, ref model.Y, cosZ, sinZ );
					Utils.RotateY( ref model.X, ref model.Z, cosY, sinY );
					Utils.RotateX( ref model.Y, ref model.Z, cosX, sinX );
				} else if( Rotate == RotateOrder.XZY ) {
					Utils.RotateX( ref model.Y, ref model.Z, cosX, sinX );
					Utils.RotateZ( ref model.X, ref model.Y, cosZ, sinZ );
					Utils.RotateY( ref model.X, ref model.Z, cosY, sinY );
				}
				
				// Rotate globally
				if( !head) {
					model.X += x; model.Y += y; model.Z += z;
					Utils.RotateY( ref model.X, ref model.Z, cosYaw, sinYaw );
				} else {
					Utils.RotateY( ref model.X, ref model.Z, cosHead, sinHead );
					float tempX = x, tempZ = z;
					Utils.RotateY( ref tempX, ref tempZ, cosYaw, sinYaw );
					model.X += tempX; model.Y += y; model.Z += tempZ;
				}
				model.X += pos.X; model.Y += pos.Y; model.Z += pos.Z;
				
				FastColour col = part.Count == boxVertices ? 
					cols[i >> 2] : FastColour.Scale( this.col, 0.7f );
				VertexP3fT2fC4b vertex = default( VertexP3fT2fC4b );
				vertex.X = model.X; vertex.Y = model.Y; vertex.Z = model.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				
				vertex.U = model.U / 64f; vertex.V = model.V / vScale;
				int quadI = i % 4;
				if( quadI == 0 || quadI == 3 ) vertex.V -= 0.01f / vScale;
				if( quadI == 2 || quadI == 3 ) vertex.U -= 0.01f / 64f;
				cache.vertices[index++] = vertex;
			}
		}
		
		protected enum RotateOrder { ZYX, XZY }
	}
}