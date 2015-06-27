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
				
		Vector3 pos;
		SelectionBoxComparer comparer = new SelectionBoxComparer();
		public void Render( double delta ) {
			Player player = Window.LocalPlayer;
			pos = player.Position;
			if( selections.Count == 0 ) return;
			// TODO: Proper selection box sorting. But this is very difficult because
			// we can have boxes within boxes, intersecting boxes, etc..
			comparer.pos = pos;
			selections.Sort( comparer );
			
			Graphics.BeginVbBatch( VertexFormat.Pos3fCol4b );
			Graphics.AlphaBlending = true;
			for( int i = 0; i < selections.Count; i++ ) {
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