using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace PluginBeta173 {
	
	public sealed class Beta173EntityManager : EntityManager {
		
		public Dictionary<int, Entity> entities = new Dictionary<int, Entity>();
		
		public Beta173EntityManager( Game window ) : base( window ) {
		}
		
		public override void Init() {
		}
		
		public override void AddEntity( int entityId, Entity entity ) {
			entities[entityId] = entity;
			entity.EntityId = entityId;
			game.RaiseEntityAdded( entityId );
		}
		
		public override void RemoveEntities( params int[] ids ) {
			for( int i = 0; i < ids.Length; i++ ) {
				Entity entity;
				if( entities.TryGetValue( ids[i], out entity ) ) {
					entity.Despawn();
					game.RaiseEntityRemoved( ids[i] );
				}
				entities.Remove( ids[i] );
			}
		}
		
		public override void RemoveEntity( int id ) {
			Entity entity;
			if( entities.TryGetValue( id, out entity ) ) {
				entity.Despawn();
				game.RaiseEntityRemoved( id );
			}
			entities.Remove( id );
		}
		
		public override Entity this[int entityId] {
			get {
				return entities[entityId];
			}
			set {
				entities[entityId] = value;
				value.EntityId = entityId;
			}
		}
		
		public override void Tick( double period ) {
			foreach( var pair in entities ) {
				pair.Value.Tick( period );
			}
		}
		
		public override void Render( double deltaTime, float t ) {
			foreach( var pair in entities ) {
				pair.Value.Render( deltaTime, t );
			}
		}
		
		public override void Dispose() {
			foreach( var pair in entities ) {
				pair.Value.Despawn();
			}
		}
		
		public override IEnumerable<Entity> Entities {
			get {
				foreach( var pair in entities ) {
					yield return pair.Value;
				}
			}
		}
	}
}