// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities.Mobs {

	public abstract class AI {
		
		protected Game game;
		protected Entity entity;
		static Random random = new Random();
		
		public AI(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		public abstract void Tick(Entity target);
		
		public abstract void AttackedBy(Entity source);
		
		protected static void MoveRandomly(Entity source) {
			double dirX = random.NextDouble() - 0.5, dirZ = random.NextDouble() - 0.5;
			MoveInDirection(source, new Vector3((float)dirX, 0, (float)dirZ));
		}
		
		protected static void MoveInDirection(Entity source, Vector3 dir) {
			double rotYRadians, headXRadians;
			Utils.GetHeading(dir, out rotYRadians, out headXRadians);
			
			float rotY =  (float)(rotYRadians * Utils.Rad2Deg);
			float headX = (float)(headXRadians * Utils.Rad2Deg);
			LocationUpdate update = LocationUpdate.MakePosAndOri(dir, rotY, headX, true);
			source.SetLocation(update, false);
		}
	}
}