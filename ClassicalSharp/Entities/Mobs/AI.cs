// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities.Mobs {

	public abstract class AI {
		
		protected Game game;
		protected Entity entity;
		static Random random = new Random();
		
		public Vector3 MoveVelocity;
		
		public AI(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		public abstract void Tick(Entity target);
		
		public abstract void AttackedBy(Entity source);
		
		
		int randomCount;
		Vector3 randomDir;
		protected void MoveRandomly(Entity source) {
			// Move in new direction
			if (randomCount == 0) {
				randomDir.X = (float)(random.NextDouble() - 0.5);
				randomDir.Z = (float)(random.NextDouble() - 0.5);
			}
			
			randomCount = (randomCount + 1) & 0x1F;
			MoveInDirection(source, randomDir);
		}
		
		protected void MoveInDirection(Entity source, Vector3 dir) {
			double rotYRadians, headXRadians;
			Utils.GetHeading(dir, out rotYRadians, out headXRadians);
			
			float rotY =  (float)(rotYRadians * Utils.Rad2Deg);
			float headX = (float)(headXRadians * Utils.Rad2Deg);
			LocationUpdate update = LocationUpdate.MakeOri(rotY, headX);
			source.SetLocation(update, false);
			MoveVelocity = dir * 0.9f;
		}
	}
	
	public sealed class FleeAI : AI {
		
		public FleeAI(Game game, Entity entity) : base(game, entity) { }
		
		public override void Tick(Entity target) {
			MoveRandomly(entity);
		}
		
		public override void AttackedBy(Entity source) {
			Vector3 fleeDir = -Vector3.Normalize(source.Position - entity.Position);
			MoveInDirection(source, fleeDir * 5);
		}
	}
	
	public sealed class HostileAI : AI {
		
		public HostileAI(Game game, Entity entity) : base(game, entity) { }
		
		public override void Tick(Entity target) {
			float distSq = (target.Position - entity.Position).LengthSquared;
			if (distSq > 16 * 16) {
				MoveRandomly(entity);
			} else {
				Vector3 dir = Vector3.Normalize(target.Position - entity.Position);
				dir.Y = 0;
				MoveInDirection(entity, dir);
			}
		}
		
		public override void AttackedBy(Entity source) {
		}
	}
}