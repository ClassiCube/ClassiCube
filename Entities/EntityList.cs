using System;
using OpenTK;

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
		
		public byte GetClosetPlayer( Vector3 eyePos, Vector3 dir ) {
			float closestDist = float.PositiveInfinity;
			byte targetId = 255;
			
			for( int i = 0; i < Players.Length - 1; i++ ) { // -1 because we don't want to pick against local player
				Player p = Players[i];
				if( p == null ) continue;
				float t0, t1;
				if( Intersection.RayIntersectsRotatedBox( eyePos, dir, p, out t0, out t1 ) ) {
					if( t0 < closestDist ) {
						closestDist = t0;
						targetId = (byte)i;
					}
				}
			}
			return targetId;
		}
		
		public Player this[int id] {
			get { return Players[id]; }
			set { 
				Players[id] = value;
				value.ID = (byte)id;
			}
		}
	}
}
