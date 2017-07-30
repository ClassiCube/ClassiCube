// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Renderers {
	
	public class HeldBlockRenderer : IGameComponent {
		
		internal BlockID type;
		IModel model;
		internal HeldBlockAnimation anim;
		
		Game game;
		internal FakePlayer held;
		Matrix4 heldBlockProj;
		
		public void Init(Game game) {
			this.game = game;
			model = game.ModelCache.Get("0");
			held = new FakePlayer(game);
			game.Events.ProjectionChanged += ProjectionChanged;
			
			anim = new HeldBlockAnimation();
			anim.Init(game, this);
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		public void Render(double delta) {
			if (game.Camera.IsThirdPerson || !game.ShowBlockInHand) return;

			Vector3 last = anim.pos;
			anim.pos = Vector3.Zero;
			type = game.Inventory.Selected;
			held.RotX = 0;
			if (anim.doAnim) anim.Update(delta, last);
			
			game.Graphics.SetMatrixMode(MatrixType.Projection);
			game.Graphics.LoadMatrix(ref heldBlockProj);
			game.Graphics.SetMatrixMode(MatrixType.Modelview);
			SetMatrix();
			
			game.Graphics.Texturing = true;
			game.Graphics.SetupAlphaState(game.BlockInfo.Draw[type]);
			game.Graphics.DepthTest = false;
			
			SetPos();
			game.Graphics.FaceCulling = true;
			model.Render(held);
			game.Graphics.FaceCulling = false;
			
			game.Graphics.LoadMatrix(ref game.View);
			game.Graphics.SetMatrixMode(MatrixType.Projection);
			game.Graphics.LoadMatrix(ref game.Projection);
			game.Graphics.SetMatrixMode(MatrixType.Modelview);
			
			game.Graphics.Texturing = false;
			game.Graphics.RestoreAlphaState(game.BlockInfo.Draw[type]);
			game.Graphics.DepthTest = true;
		}
		
		static Vector3 nOffset = new Vector3(0.56f, -0.72f, -0.72f);
		static Vector3 sOffset = new Vector3(0.46f, -0.52f, -0.72f);
		void SetMatrix() {
			Player p = game.LocalPlayer;
			Vector3 eyePos = p.EyePosition;
			Matrix4 m = Matrix4.LookAt(eyePos, eyePos - Vector3.UnitZ, Vector3.UnitY) * game.Camera.tiltM;
			game.Graphics.LoadMatrix(ref m);
		}
		
		void SetPos() {
			// Based off details from http://pastebin.com/KFV0HkmD (Thanks goodlyay!)
			BlockInfo info = game.BlockInfo;
			bool sprite = info.Draw[type] == DrawType.Sprite;
			Vector3 offset = sprite ? sOffset : nOffset;
			Player p = game.LocalPlayer;
			held.ModelScale = new Vector3(0.4f);
			
			held.Position = p.EyePosition + anim.pos;
			held.Position += offset;
			if (!sprite) {
				float height = info.MaxBB[type].Y - info.MinBB[type].Y;
				held.Position.Y += 0.2f * (1 - height);
			}
			
			held.Position.X -= game.Camera.bobbingHor;
			held.Position.Y -= game.Camera.bobbingVer;
			held.Position.Z -= game.Camera.bobbingHor;
			
			held.HeadY = -45 + anim.angleY;
			held.RotY = -45 + anim.angleY;
			held.HeadX = 0;
			held.Block = type;
		}
		
		void ProjectionChanged(object sender, EventArgs e) {
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			heldBlockProj = Matrix4.CreatePerspectiveFieldOfView(70 * Utils.Deg2Rad,
			                                                     aspectRatio, zNear, game.ViewDistance);
		}
		
		public void Dispose() {
			game.Events.ProjectionChanged -= ProjectionChanged;
			anim.Dispose();
		}
	}
	
	/// <summary> Skeleton implementation of a player entity so we can reuse
	/// block model rendering code. </summary>
	internal class FakePlayer : Player {
		
		public FakePlayer(Game game) : base(game) {
			NoShade = true;
		}
		public BlockID Block;
		
		public override void SetLocation(LocationUpdate update, bool interpolate) { }
		
		public override void Tick(double delta) { }

		public override void RenderModel(double deltaTime, float t) { }
		
		public override void RenderName() { }
		
		public override int Colour() {
			Player realP = game.LocalPlayer;
			int col = realP.Colour();
			
			// Adjust pitch so angle when looking straight down is 0.
			float adjHeadX = realP.HeadX - 90;
			if (adjHeadX < 0) adjHeadX += 360;
			
			// Adjust colour so held block is brighter when looking straght up
			float t = Math.Abs(adjHeadX - 180) / 180;
			float colScale = Utils.Lerp(0.9f, 0.7f, t);
			return FastColour.ScalePacked(col, colScale);
		}
	}
}