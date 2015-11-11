using System;
using OpenTK;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public class EntityList : IDisposable {
		
		public const int MaxCount = 256;
		public Player[] Players = new Player[MaxCount];
		
		/// <summary> Performs a tick call for all player entities contained in this list. </summary>
		public void Tick( double delta ) {
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null ) {
					Players[i].Tick( delta );
				}
			}
		}
		
		/// <summary> Renders the models of all player entities contained in this list. </summary>
		public void RenderModels( IGraphicsApi api, double delta, float t ) {
			api.Texturing = true;
			api.AlphaTest = true;
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null )
					Players[i].RenderModel( delta, t );
			}
			api.Texturing = false;
			api.AlphaTest = false;
		}
		
		/// <summary> Renders the names of all player entities contained in this list. </summary>
		public void RenderNames( IGraphicsApi api, double delta, float t ) {
			api.Texturing = true;
			api.AlphaTest = true;
			for( int i = 0; i < Players.Length; i++ ) {
				if( Players[i] != null )
					Players[i].RenderName();
			}
			api.Texturing = false;
			api.AlphaTest = false;
		}
		
		/// <summary> Disposes of all player entities contained in this list. </summary>
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
		
		/// <summary> Gets or sets the player entity for the specified id. </summary>
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
