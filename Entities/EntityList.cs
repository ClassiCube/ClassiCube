using System;

namespace ClassicalSharp {

	public class EntityList {
		
		public int MaxCount = 256;
		public Player[] Players = new Player[256];
		
		public void Tick( double delta ) {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null ) {
					Players[i].Tick( delta );
				}
			}
		}
		
		public void Render( double delta, float t ) {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null ) {
					Players[i].Render( delta, t );
				}
			}
		}
		
		public void Dispose() {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null ) {
					Players[i].Despawn();
				}
			}
		}
		
		public Player this[int id] {
			get { return Players[id]; }
			set { Players[id] = value; }
		}
	}
}
