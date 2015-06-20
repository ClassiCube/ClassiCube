using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {
	
	public class SelectionManager : IDisposable {
		
		public Game Window;
		public OpenGLApi api;
		SelectionShader shader;
		
		public SelectionManager( Game window ) {
			Window = window;
			api = window.Graphics;
		}
		
		List<SelectionBox> selections = new List<SelectionBox>( 256 );
		public void AddSelection( byte id, Vector3I p1, Vector3I p2, FastColour col ) {
			SelectionBox selection = new SelectionBox( p1, p2, col, api );
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
		
		unsafe void Intersect( SelectionBox box, Vector3 origin, ref float closest, ref float furthest ) {
			Vector3I min = box.Min;
			Vector3I max = box.Max;
			// Bottom corners
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, min.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, min.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, min.Y, max.Z, ref closest, ref furthest );
			// top corners
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, max.Y, min.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, max.X, max.Y, max.Z, ref closest, ref furthest );
			UpdateDist( pos.X, pos.Y, pos.Z, min.X, max.Y, max.Z, ref closest, ref furthest );
		}
		
		static void UpdateDist( float x1, float y1, float z1, float x2, float y2, float z2,
		                          ref float closest, ref float furthest ) {
			float dist = Utils.DistanceSquared( x1, y1, z1, x2, y2, z2 );
			if( dist < closest ) closest = dist;
			if( dist > furthest ) furthest = dist;
		}
		
		// Basically sorts selection boxes back to front for better transparency.
		// TODO: Proper selection box sorting.
		Vector3 pos;
		int Compare( SelectionBox a, SelectionBox b ) {
			float minDistA = float.PositiveInfinity, minDistB = float.PositiveInfinity;
			float maxDistA = float.NegativeInfinity, maxDistB = float.NegativeInfinity;
			Intersect( a, pos, ref minDistA, ref maxDistA );
			Intersect( b, pos, ref minDistB, ref maxDistB );
			return maxDistA == maxDistB ? minDistA.CompareTo( minDistB ) : maxDistA.CompareTo( maxDistB );
		}
		
		public void Init() {
			shader = new SelectionShader();
			shader.Init( api );
		}
		
		public void Render( double delta ) {
			if( selections.Count == 0 ) return;
			
			api.UseProgram( shader.ProgramId );
			shader.UpdateFogAndMVPState( ref Window.MVP );
			
			Player player = Window.LocalPlayer;
			pos = player.Position;
			selections.Sort( (a, b) => Compare( a, b ) );	
			
			api.AlphaBlending = true;
			for( int i = 0 ; i < selections.Count; i++ ) {
				SelectionBox box = selections[i];
				box.Render( shader );
			}
			api.AlphaBlending = false;
		}
		
		public void Dispose() {
			foreach( SelectionBox sel in selections ) {
				sel.Dispose();
			}
			selections.Clear();
		}
	}
}