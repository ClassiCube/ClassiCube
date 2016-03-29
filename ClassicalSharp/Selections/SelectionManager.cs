// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {
	
	public class SelectionManager : IDisposable {
		
		protected Game game;
		public IGraphicsApi Graphics;
		VertexP3fC4b[] vertices, lineVertices;
		int vb, lineVb;
		
		public SelectionManager( Game window ) {
			game = window;
			Graphics = window.Graphics;
			window.WorldEvents.OnNewMap += OnNewMap;
		}
		
		List<SelectionBox> selections = new List<SelectionBox>( 256 );
		public void AddSelection( byte id, Vector3I p1, Vector3I p2, FastColour col ) {
			RemoveSelection( id );
			SelectionBox selection = new SelectionBox( p1, p2, col );
			selection.ID = id;
			selections.Add( selection );
		}
		
		public void RemoveSelection( byte id ) {
			for( int i = 0; i < selections.Count; i++ ) {
				SelectionBox sel = selections[i];
				if( sel.ID == id ) {
					selections.RemoveAt( i );
					break;
				}
			}
		}
		
		SelectionBoxComparer comparer = new SelectionBoxComparer();
		public void Render( double delta ) {
			if( selections.Count == 0 ) return;			
			
			// TODO: Proper selection box sorting. But this is very difficult because
			// we can have boxes within boxes, intersecting boxes, etc. Probably not worth it.
			Vector3 camPos = game.CurrentCameraPos;
			for( int i = 0; i < selections.Count; i++ )
				comparer.Intersect( selections[i], camPos );				
			selections.Sort( comparer );
			if( vertices == null )
				InitData(); // lazy init as most servers don't use this.
			
			int index = 0, lineIndex = 0;
			for( int i = 0; i < selections.Count; i++ ) {
				SelectionBox box = selections[i];
				box.Render( delta, vertices, lineVertices, ref index, ref lineIndex );
			}
			
			Graphics.SetBatchFormat( VertexFormat.Pos3fCol4b );
			Graphics.UpdateDynamicVb( DrawMode.Lines, lineVb, lineVertices, selections.Count * LineVerticesCount );
			
			Graphics.DepthWrite = false;
			Graphics.AlphaBlending = true;
			Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, vb, vertices,
			                                selections.Count * VerticesCount, selections.Count * IndicesCount );
			Graphics.DepthWrite = true;
			Graphics.AlphaBlending = false;
		}
		
		public void Dispose() {
			OnNewMap( null, null );
			game.WorldEvents.OnNewMap -= OnNewMap;
			if( lineVb > 0 ) {
				Graphics.DeleteDynamicVb( vb );
				Graphics.DeleteDynamicVb( lineVb );
			}
		}
		
		const int VerticesCount = 6 * 4, LineVerticesCount = 12 * 2, IndicesCount = 6 * 6;
		void InitData() {
			vertices = new VertexP3fC4b[256 * VerticesCount];
			lineVertices = new VertexP3fC4b[256 * LineVerticesCount];
			vb = Graphics.CreateDynamicVb( VertexFormat.Pos3fCol4b, vertices.Length );
			lineVb = Graphics.CreateDynamicVb( VertexFormat.Pos3fCol4b, lineVertices.Length );
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			selections.Clear();
		}
	}
}