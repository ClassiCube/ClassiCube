using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class Particle : IDisposable {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public Vector2 Size = new Vector2( 0.0625f, 0.0625f );
		public double Lifetime = 0;
		public Game Window;
		protected Vector3 lastPos, nextPos;

		public abstract void Render( double delta, float t, VertexPos3fTex2f[] vertices, ref int index );
		
		public abstract void Dispose();
		
		public Particle( Game window, Vector3 pos, Vector3 velocity, double lifetime ) {
			Window = window;
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = lifetime;
		}
		
		public virtual bool Tick( double delta ) {
			Lifetime -= delta;
			return Lifetime < 0;
		}
	}
}