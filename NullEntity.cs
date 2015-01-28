using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public class NullEntity : Entity {
		
		public NullEntity( Game game ) : base( game ) {			
		}
		
		public override Vector3 Size {
			get { return new Vector3( 1f, 1f, 1f ); }
		}
		
		public override float StepSize {
			get { return 1f; }
		}
		
		public override void Despawn() {
			
		}
		
		public override void Render( double deltaTime, float t ) {
			Vector3 pos = Position;
			VertexPos3fCol4b[] vertices = {
				new VertexPos3fCol4b( pos.X + 1, pos.Y, pos.Z, FastColour.White ),
				new VertexPos3fCol4b( pos.X + 1, pos.Y + 1, pos.Z, FastColour.White ),
				new VertexPos3fCol4b( pos.X, pos.Y, pos.Z, FastColour.White ),
			};
			game.Graphics.DrawVertices( DrawMode.Triangles, vertices );
		}
				
		public override void Tick( double delta ) {
			
		}
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			if( update.IncludesPosition ) {
				Position = update.RelativePosition ? Position + update.Pos : update.Pos;
			}
			if( update.IncludesOrientation ) {
				YawDegrees = update.Yaw;
				PitchDegrees = update.Pitch;
			}
		}
	}
}