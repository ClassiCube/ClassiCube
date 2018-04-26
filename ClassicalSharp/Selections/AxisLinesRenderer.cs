// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Selections {

	public class AxisLinesRenderer : IGameComponent {
		
		VertexP3fC4b[] vertices;
		int vb;
		Game game;
		const float size = 1/32f;
		
		void IGameComponent.Init(Game game) { 
			this.game = game;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		void IDisposable.Dispose() { 
			ContextLost();
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void IGameComponent.Ready(Game game) { }			
		void IGameComponent.Reset(Game game) { }
		void IGameComponent.OnNewMap(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }				
		
		public void Render(double delta) {
			if (!game.ShowAxisLines || game.Graphics.LostContext) return;
			if (vertices == null) {
				vertices = new VertexP3fC4b[12];
				ContextRecreated();
			}
			game.Graphics.Texturing = false;
			Vector3 P = game.LocalPlayer.Position; P.Y += 0.05f;
			int index = 0;
			
			SelectionBox.HorQuad(vertices, ref index, FastColour.Red.Pack(), 
			                   P.X, P.Z - size, P.X + 3, P.Z + size, P.Y);
			SelectionBox.HorQuad(vertices, ref index, FastColour.Blue.Pack(),
			                   P.X - size, P.Z, P.X + size, P.Z + 3, P.Y);
			if (game.Camera.IsThirdPerson) {
				SelectionBox.VerQuad(vertices, ref index, FastColour.Green.Pack(),
				                     P.X - size, P.Y, P.Z + size, P.X + size, P.Y + 3, P.Z - size);
			}
			
			game.Graphics.SetBatchFormat(VertexFormat.P3fC4b);
			game.Graphics.UpdateDynamicVb_IndexedTris(vb, vertices, index);
		}
		
		void ContextLost() { game.Graphics.DeleteVb(ref vb); }
		
		void ContextRecreated() {
			if (vertices == null) return;
			vb = game.Graphics.CreateDynamicVb(VertexFormat.P3fC4b, vertices.Length);
		}
	}
}
