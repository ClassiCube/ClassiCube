using System;
using System.Collections.Generic;

namespace ClassicalSharp.Entities {
	
	public class EntityManager {
		
		public Dictionary<int, Entity> entities = new Dictionary<int, Entity>();
		public Game game;
		
		public EntityManager( Game game ) {
			this.game = game;
		}
		
		public void AddEntity( int entityId, Entity entity ) {
			entities[entityId] = entity;
			entity.EntityId = entityId;
		}
		
		public void RemoveEntities( params int[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				entities.Remove( ids[i] );
			}
		}
		
		public void RemoveEntity( int id ) {
			entities.Remove( id );
		}
		
		public Entity this[int entityId] {
			get { 
				return entities[entityId];
			}
			set { 
				entities[entityId] = value;
				value.EntityId = entityId;
			}
		}
		
		public void Tick( double period ) {
			foreach( var pair in entities ) {
				pair.Value.Tick( period );
			}
		}
		
		public void Render( double deltaTime, float t ) {
			foreach( var pair in entities ) {
				pair.Value.Render( deltaTime, t );
			}
		}
		
		public void Dispose() {
			foreach( var pair in entities ) {
				pair.Value.Despawn();
			}
		}
	}
}