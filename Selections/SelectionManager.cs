using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {
	
	public class SelectionManager : IDisposable {
		
		public Game Window;
		public IGraphicsApi Graphics;
		
		public SelectionManager( Game window ) {
			Window = window;
			Graphics = window.Graphics;
		}
		
		List<SelectionBox> selections = new List<SelectionBox>( 256 );
		public void AddSelection( byte id, Vector3I p1, Vector3I p2, FastColour col ) {
			SelectionBox selection = new SelectionBox( p1, p2, col, Graphics );
			selection.ID = id;
			selections.Add( selection );
		}
		
		public void RemoveSelection( byte id ) {
			for( int i = 0; i < selections.Count; i++ ) {
				SelectionBox sel = selections[i];
				if( sel.ID == id ) {
					sel.Dispose();
					selections.RemoveAt( i );
					break;
				}
			}
		}
		
		void Intersect( SelectionBox box, Vector3 origin, out float minDist, out float maxDist ) {
			Vector3I min = box.Min;
			Vector3I max = box.Max;
			
			float closest = float.PositiveInfinity;
			float furthest = float.NegativeInfinity;
			foreach( float dist in GetDistances( origin, min, max ) ) {
				if( dist < closest ) closest = dist;
				if( dist > furthest ) furthest = dist;
			}
			minDist = closest;
			maxDist = furthest;
		}
		
		static IEnumerable<float> GetDistances( Vector3 pos, Vector3I min, Vector3I max ) {
			// bottom corners
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, min.X, min.Y, min.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, max.X, min.Y, min.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, max.X, min.Y, max.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, min.X, min.Y, max.Z );
			// top corners
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, min.X, max.Y, min.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, max.X, max.Y, min.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, max.X, max.Y, max.Z );
			yield return Utils.DistanceSquared( pos.X, pos.Y, pos.Z, min.X, max.Y, max.Z );
		}
		
		// Basically sorts selection boxes back to front for better transparency.
		// TODO: Proper selection box sorting.
		Vector3 pos;
		int Compare( SelectionBox a, SelectionBox b ) {
			float minDistA = 0, minDistB = 0, maxDistA = 0, maxDistB = 0;
			Intersect( a, pos, out minDistA, out maxDistA );
			Intersect( b, pos, out minDistB, out maxDistB );
			return maxDistA == maxDistB ? minDistA.CompareTo( minDistB ) : maxDistA.CompareTo( maxDistB );
		}
		
		public void Render( double delta ) {
			Player player = Window.LocalPlayer;
			pos = player.Position;
			if( selections.Count == 0 ) return;
			selections.Sort( (a, b) => Compare( a, b ) );
			
			Graphics.BeginVbBatch( VertexFormat.VertexPos3fCol4b );
			Graphics.AlphaBlending = true;
			for( int i = 0 ; i < selections.Count; i++ ) {
				SelectionBox box = selections[i];
				box.Render( delta );
			}
			Graphics.AlphaBlending = false;
			Graphics.EndVbBatch();
		}
		
		public void Dispose() {
			foreach( SelectionBox sel in selections ) {
				sel.Dispose();
			}
			selections.Clear();
		}
	}
}