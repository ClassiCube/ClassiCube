using System;
using OpenTK;

namespace ClassicalSharp.Particles {

	public abstract class Particle : IDisposable {
		
		public Vector3 Position;
		public Vector3 Velocity;
		public TextureRec Rectangle;
		public float Lifetime;
		protected Game game;
		protected Vector3 lastPos, nextPos;

		public abstract void Render( double delta, float t, VertexPos3fTex2fCol4b[] vertices, ref int index );
		
		public abstract void Dispose();
		
		public Particle( Game game, Vector3 pos, Vector3 velocity, double lifetime, TextureRec rec ) {
			this.game = game;
			Position = lastPos = nextPos = pos;
			Velocity = velocity;
			Lifetime = (float)lifetime;
			Rectangle = rec;
		}
		
		public virtual bool Tick( double delta ) {
			Lifetime -= (float)delta;
			return Lifetime < 0;
		}
	}
}