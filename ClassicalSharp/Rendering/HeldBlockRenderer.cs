// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class HeldBlockRenderer : IGameComponent {
		
		internal byte type;
		internal BlockModel block;
		internal HeldBlockAnimation anim;
		
		Game game;		
		FakePlayer held;
		Matrix4 heldBlockProj;		
		
		public void Init(Game game) {
			this.game = game;
			block = new BlockModel(game);
			block.NoShade = true;
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
			type = game.Inventory.HeldBlock;
			block.CosX = 1; block.SinX = 0;
			block.SwitchOrder = false;			
			if (anim.doAnim) anim.Update(delta, last);
			
			SetMatrix();
			game.Graphics.SetMatrixMode(MatrixType.Projection);
			game.Graphics.LoadMatrix(ref heldBlockProj);
			bool translucent = game.BlockInfo.Draw[type] == DrawType.Translucent;
			
			game.Graphics.Texturing = true;
			if (translucent) game.Graphics.AlphaBlending = true;
			else game.Graphics.AlphaTest = true;
			game.Graphics.DepthTest = false;
			
			SetPos();
			block.Render(held);
			
			game.Graphics.LoadMatrix(ref game.Projection);
			game.Graphics.SetMatrixMode(MatrixType.Modelview);
			game.Graphics.LoadMatrix(ref game.View);
			
			game.Graphics.Texturing = false;
			if (translucent) game.Graphics.AlphaBlending = false;
			else game.Graphics.AlphaTest = false;
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
			held.ModelScale = 0.4f;
		   
			held.Position = p.EyePosition + anim.pos;
			held.Position += offset;
			if (!sprite) {
				float height = info.MaxBB[type].Y - info.MinBB[type].Y;
				held.Position.Y += 0.2f * (1 - height);
			}
		   
			held.Position.X -= game.Camera.bobbingHor;
			held.Position.Y -= game.Camera.bobbingVer;
			held.Position.Z -= game.Camera.bobbingHor;
			   
			held.HeadYawDegrees = -45 + anim.angleY;
			held.YawDegrees = -45 + anim.angleY;
			held.PitchDegrees = 0;
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
		}
		public byte Block;
		
		public override void SetLocation(LocationUpdate update, bool interpolate) { }
		
		public override void Tick(double delta) { }

		public override void RenderModel(double deltaTime, float t) { }
		
		public override void RenderName() { }
	}
}