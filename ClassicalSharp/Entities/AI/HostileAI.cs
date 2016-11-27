// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public sealed class HostileAI : AI {
		
		public HostileAI(Game game, Entity entity) : base(game, entity) { }
		
		public override void Tick(Entity target) {
			float distSq = (target.Position - entity.Position).LengthSquared;
			if (distSq > 32 * 32) {
				MoveRandomly(entity);
			} else {
				Vector3 dir = Vector3.Normalize(target.Position - entity.Position);
				MoveInDirection(entity, dir);
			}
		}
		
		public override void AttackedBy(Entity source) {
		}
	}
}
