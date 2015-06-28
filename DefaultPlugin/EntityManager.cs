using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace DefaultPlugin {

	public class StandardEntityManager : EntityManager {
		
		Entity[] entities = new Entity[256];
		public StandardEntityManager( Game window ) : base( window ) {
		}
		
		public override void Init() {
			entities[255] = game.LocalPlayer;
		}
		
		public override void AddEntity( int entityId, Entity entity ) {
			entities[entityId] = entity;
			entity.EntityId = entityId;
			game.RaiseEntityAdded( entityId );
		}
		
		public override void RemoveEntities( params int[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				RemoveEntity( ids[i] );
			}
		}
		
		public override void RemoveEntity( int id ) {
			Entity entity = entities[id];
			if( entity != null ) {
				entity.Despawn();
				game.RaiseEntityRemoved( id );
			}
			entities[id] = null;
		}
		
		public override Entity this[int entityId] {
			get { return entities[entityId]; }
			set {
				entities[entityId] = value;
				value.EntityId = entityId;
			}
		}
		
		public override void Tick( double period ) {
			for( int i = 0; i < entities.Length; i++ ) {
				Entity entity = entities[i];
				if( entity != null ) {
					entity.Tick( period );
				}
			}
		}
		
		public override void Render( double deltaTime, float t ) {
			for( int i = 0; i < entities.Length; i++ ) {
				Entity entity = entities[i];
				if( entity != null ) {
					entity.Render( deltaTime, t );
				}
			}
		}
		
		public override void Dispose() {
			for( int i = 0; i < entities.Length; i++ ) {
				Entity entity = entities[i];
				if( entity != null ) {
					entity.Despawn();
				}
			}
		}
		
		public override IEnumerable<Entity> Entities {
			get {
				for( int i = 0; i < entities.Length; i++ ) {
					Entity entity = entities[i];
					if( entity != null ) {
						yield return entity;
					}
				}
			}
		}
	}
}
