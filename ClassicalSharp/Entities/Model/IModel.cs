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
		
		/// <summary> The maximum scale the entity can have (for collisions and rendering). </summary>
		public virtual float MaxScale { get { return 2; } }
		
		/// <summary> The size of the bounding box that is used when
		/// performing collision detection for this model. </summary>
		public abstract Vector3 CollisionSize { get; }
		
		/// <summary> Bounding box that contains this model,
		/// assuming that the model is not rotated at all.</summary>
		public abstract AABB PickingBounds { get; }
		
		protected Vector3 pos;
		protected float cosYaw, sinYaw, cosHead, sinHead;
		protected float uScale, vScale, scale;
		
		/// <summary> Returns whether the model should be rendered based on the given entity's position. </summary>
		public virtual bool ShouldRender( Player p, FrustumCulling culling ) {
			Vector3 pos = p.Position;
			AABB bb = PickingBounds;
			float maxLen = Math.Max( bb.Width, Math.Max( bb.Height, bb.Length ) );
			maxLen *= p.ModelScale;
			return culling.SphereInFrustum( pos.X, pos.Y + maxLen / 2, pos.Z, maxLen );
		}
		
		/// <summary> Renders the model based on the given entity's position and orientation. </summary>
		public void Render( Player p ) {
			index = 0;
			pos = p.Position;
			if( Bobbing ) pos.Y += p.anim.bobYOffset;
			World map = game.World;
			col = game.World.IsLit( p.EyePosition ) ? map.Env.Sunlight : map.Env.Shadowlight;
			uScale = 1 / 64f; vScale = 1 / 32f;
			scale = p.ModelScale;
			
			cols[0] = col;
			cols[1] = FastColour.Scale( col, FastColour.ShadeYBottom );
			cols[2] = FastColour.Scale( col, FastColour.ShadeZ ); cols[3] = cols[2];
			cols[4] = FastColour.Scale( col, FastColour.ShadeX ); cols[5] = cols[4];
			
			cosYaw = (float)Math.Cos( p.YawDegrees * Utils.Deg2Rad );
			sinYaw = (float)Math.Sin( p.YawDegrees * Utils.Deg2Rad );
			cosHead = (float)Math.Cos( p.HeadYawDegrees * Utils.Deg2Rad );
			sinHead = (float)Math.Sin( p.HeadYawDegrees * Utils.Deg2Rad );

			graphics.SetBatchFormat( VertexFormat.P3fT2fC4b );
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
		
		protected void DrawPart( ModelPart part ) {
			VertexP3fT2fC4b vertex = default( VertexP3fT2fC4b );
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex v = vertices[part.Offset + i];
				float t = cosYaw * v.X - sinYaw * v.Z; v.Z = sinYaw * v.X + cosYaw * v.Z; v.X = t; // Inlined RotY
				v.X *= scale; v.Y *= scale; v.Z *= scale;
				v.X += pos.X; v.Y += pos.Y; v.Z += pos.Z;
				
				FastColour col = part.Count == boxVertices ? 
					cols[i >> 2] : FastColour.Scale( this.col, 0.7f );
				vertex.X = v.X; vertex.Y = v.Y; vertex.Z = v.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				
				vertex.U = v.U * uScale; vertex.V = v.V * vScale;
				int quadI = i & 3;
				if( quadI == 0 || quadI == 3 ) vertex.V -= 0.01f * vScale;
				if( quadI == 2 || quadI == 3 ) vertex.U -= 0.01f * uScale;
				cache.vertices[index++] = vertex;
			}
		}
		
		protected void DrawRotate( float angleX, float angleY, float angleZ, ModelPart part ) {
			DrawRotated( angleX, angleY, angleZ, part, false );
		}
		
		protected void DrawHeadRotate( float angleX, float angleY, float angleZ, ModelPart part ) {
			DrawRotated( angleX, angleY, angleZ, part, true );
		}
		
		protected void DrawRotated( float angleX, float angleY, float angleZ, ModelPart part, bool head ) {
			float cosX = (float)Math.Cos( -angleX ), sinX = (float)Math.Sin( -angleX );
			float cosY = (float)Math.Cos( -angleY ), sinY = (float)Math.Sin( -angleY );
			float cosZ = (float)Math.Cos( -angleZ ), sinZ = (float)Math.Sin( -angleZ );
			float x = part.RotX, y = part.RotY, z = part.RotZ;
			VertexP3fT2fC4b vertex = default( VertexP3fT2fC4b );
			
			for( int i = 0; i < part.Count; i++ ) {
				ModelVertex v = vertices[part.Offset + i];
				v.X -= x; v.Y -= y; v.Z -= z;
				float t = 0;
				
				// Rotate locally
				if( Rotate == RotateOrder.ZYX ) {
					t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; // Inlined RotZ
					t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t;  // Inlined RotY
					t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; // Inlined RotX
				} else if( Rotate == RotateOrder.XZY ) {
					t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; // Inlined RotX
					t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; // Inlined RotZ
					t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t;  // Inlined RotY
				}
				
				// Rotate globally
				if( !head ) {
					v.X += x; v.Y += y; v.Z += z;
					t = cosYaw * v.X - sinYaw * v.Z; v.Z = sinYaw * v.X + cosYaw * v.Z; v.X = t;     // Inlined RotY
				} else {
					t = cosHead * v.X - sinHead * v.Z; v.Z = sinHead * v.X + cosHead * v.Z; v.X = t; // Inlined RotY
					float tX = x, tZ = z;
					t = cosYaw * tX - sinYaw * tZ; tZ = sinYaw * tX + cosYaw * tZ; tX = t;           // Inlined RotY
					v.X += tX; v.Y += y; v.Z += tZ;
				}
				v.X *= scale; v.Y *= scale; v.Z *= scale;
				v.X += pos.X; v.Y += pos.Y; v.Z += pos.Z;
				
				FastColour col = part.Count == boxVertices ? 
					cols[i >> 2] : FastColour.Scale( this.col, 0.7f );
				vertex.X = v.X; vertex.Y = v.Y; vertex.Z = v.Z;
				vertex.R = col.R; vertex.G = col.G; vertex.B = col.B; vertex.A = 255;
				
				vertex.U = v.U * uScale; vertex.V = v.V * vScale;
				int quadI = i & 3;
				if( quadI == 0 || quadI == 3 ) vertex.V -= 0.01f * vScale;
				if( quadI == 2 || quadI == 3 ) vertex.U -= 0.01f * vScale;
				cache.vertices[index++] = vertex;
			}
		}
		
		protected enum RotateOrder { ZYX, XZY }
	}
}