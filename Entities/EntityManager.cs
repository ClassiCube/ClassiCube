using System;
using System.Collections.Generic;

namespace ClassicalSharp {
	
	public abstract class EntityManager {
		
		public Game game;
		
		public EntityManager( Game game ) {
			this.game = game;
		}
		
		public abstract void Init();
		
		public abstract void AddEntity( int entityId, Entity entity );
		
		public abstract void RemoveEntities( params int[] ids );
		
		public abstract void RemoveEntity( int id );
		
		public abstract Entity this[int entityId] { get; set; }
		
		public abstract void Tick( double period );
		
		public abstract void Render( double deltaTime, float t );
		
		public abstract void Dispose();
		
		public abstract IEnumerable<Entity> Entities { get; }
	}
}