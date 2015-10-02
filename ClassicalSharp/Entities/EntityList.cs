using System;
using OpenTK;

namespace ClassicalSharp {

	public class EntityList : IDisposable {
		
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
		
		public byte GetClosetPlayer( LocalPlayer localP ) {
			Vector3 eyePos = localP.EyePosition;
			Vector3 dir = Utils.GetDirVector( localP.YawRadians, localP.PitchRadians );
			float closestDist = float.PositiveInfinity;
			byte targetId = 255;
			
			for( int i = 0; i < Players.Length - 1; i++ ) { // -1 because we don't want to pick against local player
				Player p = Players[i];
				if( p == null ) continue;
				float t0, t1;
				if( Intersection.RayIntersectsRotatedBox( eyePos, dir, p, out t0, out t1 ) ) {
					if( t0 < closestDist && closestDist < localP.ReachDistance ) {
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
				if( value != null )
					value.ID = (byte)id;
			}
		}
	}
}
