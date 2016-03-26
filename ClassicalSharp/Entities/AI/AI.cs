// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {

	public abstract class AI {
		
		protected Game game;
		protected Entity entity;
		static Random random = new Random();
		
		public AI( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
		}
		
		public abstract void Tick( Entity target );
		
		public abstract void AttackedBy( Entity source );
		
		protected static void MoveRandomly( Entity source ) {
			double dirX = random.NextDouble() - 0.5, dirZ = random.NextDouble() - 0.5;
			MoveInDirection( source, new Vector3( (float)dirX, 0, (float)dirZ ) );
		}
		
		protected static void MoveInDirection( Entity source, Vector3 dir ) {
			double yawRad, pitchRad;
			Utils.GetHeading( dir, out yawRad, out pitchRad );
			float yaw = (float)(yawRad * Utils.Rad2Deg), pitch = (float)(pitchRad * Utils.Rad2Deg);
			LocationUpdate update = LocationUpdate.MakePosAndOri( dir, yaw, pitch, true );
			source.SetLocation( update, false );
		}
	}
}