// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public sealed class NetPlayer : Player {
		
		NetInterpComponent interp;
		public NetPlayer(string displayName, string skinName, Game game) : base(game) {
			DisplayName = displayName;
			SkinName = skinName;
			interp = new NetInterpComponent(game, this);
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		public override void Tick(double delta) {
			CheckSkin();
			tickCount++;
			interp.AdvanceState();
			anim.UpdateAnimState(interp.prev.Pos, interp.next.Pos, delta);
		}

		bool shouldRender = false;
		public override void RenderModel(double deltaTime, float t) {
			Position = Vector3.Lerp(interp.prev.Pos, interp.next.Pos, t);
			interp.LerpAngles(t);
			
			anim.GetCurrentAnimState(t);
			shouldRender = IModel.ShouldRender(this, game.Culling);
			if (shouldRender) Model.Render(this);
		}
		
		public override void RenderName() { 
			if (!shouldRender) return;
			float dist = IModel.RenderDistance(this, game.CurrentCameraPos);			
			float threshold = game.Entities.NamesMode == NameMode.AllUnscaled ? 8192 * 8192 : 32 * 32;
			if (dist <= threshold) DrawName();
		}
	}
}