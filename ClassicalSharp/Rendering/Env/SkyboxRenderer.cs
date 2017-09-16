// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public sealed class SkyboxRenderer : IGameComponent {
		
		int tex, vb = -1;
		Game game;
		const int count = 6 * 4;
		
		public bool ShouldRender {
			get { return tex > 0 && !(game.EnvRenderer.minimal); }
		}
		
		public void Init(Game game) {
			this.game = game;
			game.Events.TextureChanged += TextureChanged;
			game.Events.TexturePackChanged += TexturePackChanged;
			game.WorldEvents.EnvVariableChanged += EnvVariableChanged;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			ContextRecreated();
		}
		
		public void Reset(Game game) { game.Graphics.DeleteTexture(ref tex); }
		public void Ready(Game game) { }
		public void OnNewMap(Game game) { MakeVb(); }
		public void OnNewMapLoaded(Game game) { }
		
		public void Dispose() {
			game.Graphics.DeleteTexture(ref tex);
			ContextLost();
			
			game.Events.TextureChanged -= TextureChanged;
			game.Events.TexturePackChanged -= TexturePackChanged;
			game.WorldEvents.EnvVariableChanged -= EnvVariableChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;			
		}
		
		void EnvVariableChanged(object sender, EnvVarEventArgs e) {
			if (e.Var != EnvVar.CloudsColour) return;
			MakeVb();
		}
		
		void TexturePackChanged(object sender, EventArgs e) {
			game.Graphics.DeleteTexture(ref tex);
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name == "skybox.png")
				game.UpdateTexture(ref tex, e.Name, e.Data, false);
		}
		
		public void Render(double deltaTime) {
			if (vb == -1) return;
			game.Graphics.DepthWrite = false;
			game.Graphics.Texturing = true;
			game.Graphics.BindTexture(tex);
			game.Graphics.SetBatchFormat(VertexFormat.P3fT2fC4b);
			
			Matrix4 m = Matrix4.Identity, rotY, rotX;
			Vector2 rotation = game.Camera.GetCameraOrientation();
			
			Matrix4.RotateY(out rotY, rotation.X); // yaw
			Matrix4.Mult(out m, ref m, ref rotY);
			Matrix4.RotateX(out rotX, rotation.Y); // pitch
			Matrix4.Mult(out m, ref m, ref rotX);
			Matrix4.Mult(out m, ref m, ref game.Camera.tiltM);
			
			game.Graphics.LoadMatrix(ref m);			
			game.Graphics.BindVb(vb);
			game.Graphics.DrawVb_IndexedTris(count);
			
			game.Graphics.Texturing = false;
			game.Graphics.LoadMatrix(ref game.View);
			game.Graphics.DepthWrite = true;
		}
		
		void ContextLost() { game.Graphics.DeleteVb(ref vb); }	
		void ContextRecreated() { MakeVb(); }
		
		
		unsafe void MakeVb() {
			if (game.Graphics.LostContext) return;
			game.Graphics.DeleteVb(ref vb);
			VertexP3fT2fC4b* vertices = stackalloc VertexP3fT2fC4b[count];
			IntPtr start = (IntPtr)vertices;
			
			const float pos = 1.0f;
			VertexP3fT2fC4b v; v.Colour = game.World.Env.CloudsCol.Pack();
			
			// Render the front quad
			                        v.Z = -pos;
			v.X =  pos; v.Y = -pos;             v.U = 0.25f; v.V = 1.00f; *vertices = v; vertices++;
			v.X = -pos;                         v.U = 0.50f;              *vertices = v; vertices++;
			            v.Y =  pos;                          v.V = 0.50f; *vertices = v; vertices++;
			v.X =  pos;                         v.U = 0.25f;              *vertices = v; vertices++;
			
			// Render the left quad
			v.X =  pos;
			            v.Y = -pos; v.Z =  pos; v.U = 0.00f; v.V = 1.00f; *vertices = v; vertices++;
			                        v.Z = -pos; v.U = 0.25f;              *vertices = v; vertices++;
			            v.Y =  pos;                          v.V = 0.50f; *vertices = v; vertices++;
			                        v.Z =  pos; v.U = 0.00f;              *vertices = v; vertices++;
			
			// Render the back quad
			                        v.Z =  pos;
			v.X = -pos; v.Y = -pos;             v.U = 0.75f; v.V = 1.00f; *vertices = v; vertices++;
			v.X =  pos;                         v.U = 1.00f;              *vertices = v; vertices++;
			            v.Y =  pos;                          v.V = 0.50f; *vertices = v; vertices++;
			v.X = -pos;                         v.U = 0.75f;              *vertices = v; vertices++;
			
			// Render the right quad
			v.X = -pos;
			            v.Y = -pos; v.Z = -pos; v.U = 0.50f; v.V = 1.00f; *vertices = v; vertices++;
			                        v.Z =  pos; v.U = 0.75f;              *vertices = v; vertices++;
			            v.Y =  pos;                          v.V = 0.50f; *vertices = v; vertices++;
			                        v.Z = -pos; v.U = 0.50f;              *vertices = v; vertices++;
			
			// Render the top quad
			            v.Y =  pos;
			v.X = -pos;             v.Z = -pos;                           *vertices = v; vertices++;
			                        v.Z =  pos;              v.V = 0.00f; *vertices = v; vertices++;
			v.X =  pos;                         v.U = 0.25f;              *vertices = v; vertices++;
			                        v.Z = -pos;              v.V = 0.50f; *vertices = v; vertices++;
			
			// Render the bottom quad
			            v.Y = -pos;
			v.X = -pos;             v.Z = -pos; v.U = 0.75f;              *vertices = v; vertices++;
			                        v.Z =  pos;              v.V = 0.00f; *vertices = v; vertices++;
			v.X =  pos;                         v.U = 0.50f;              *vertices = v; vertices++;
			                        v.Z = -pos;              v.V = 0.50f; *vertices = v; vertices++;
			
			vb = game.Graphics.CreateVb(start, VertexFormat.P3fT2fC4b, count);
		}
	}
}