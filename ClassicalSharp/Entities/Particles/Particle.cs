using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class Particle : IDisposable {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public Vector2 Size = new Vector2( 1 / 16f, 1 / 16f );
		public TextureRectangle Rectangle;
		public double Lifetime;
		protected Game game;
		protected Vector3 lastPos, nextPos;

		public abstract void Render( double delta, float t, VertexPos3fTex2f[] vertices, ref int index );
		
		public abstract void Dispose();
		
		public Particle( Game window, Vector3 pos, Vector3 velocity, double lifetime, TextureRectangle rec ) {
			game = window;
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = lifetime;
			Rectangle = rec;
		}
		
		public virtual bool Tick( double delta ) {
			Lifetime -= delta;
			return Lifetime < 0;
		}
		
		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		protected void TranslatePoints( out Vector3 p111, out Vector3 p121, out Vector3 p212, out Vector3 p222 ) {			
			Vector3 centre = Position;
			Matrix4 camera = game.View;
			Vector3 right = new Vector3( camera.Row0.X, camera.Row1.X, camera.Row2.X );
			Vector3 up = new Vector3( camera.Row0.Y, camera.Row1.Y, camera.Row2.Y );
			float x = Size.X * 8, y = Size.Y * 8;
			centre.Y += Size.Y / 2;
			
			p111 = Transform( -x, -y, ref centre, ref up, ref right );
			p121 = Transform( -x, y, ref centre, ref up, ref right );
			p212 = Transform( x, -y, ref centre, ref up, ref right );
			p222 = Transform( x, y, ref centre, ref up, ref right );
		}
		
		Vector3 Transform( float x, float y, ref Vector3 centre, ref Vector3 up, ref Vector3 right ) {
			return centre + right * x * Size.X + up * y * Size.Y;
		}
	}
}