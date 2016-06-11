// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public sealed class FleeAI : AI {
		
		public FleeAI( Game game, Entity entity ) : base( game, entity ) { }
		
		public override void Tick( Entity target ) {
			MoveRandomly( entity );
		}
		
		public override void AttackedBy( Entity source ) {
			Vector3 fleeDir = -Vector3.Normalize( source.Position - entity.Position );
			MoveInDirection( source, fleeDir * 5 );
		}
	}
}
