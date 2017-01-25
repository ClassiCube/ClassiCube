// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public sealed class NetPlayer : Player {
		
		NetInterpComponent interp;
		public NetPlayer(string displayName, string skinName, Game game, byte id) : base(game) {
			DisplayName = displayName;
			SkinName = skinName;
			SkinIdentifier = "skin_" + id;
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
			HeadY = Utils.LerpAngle(interp.prev.HeadY, interp.next.HeadY, t);
			RotY = Utils.LerpAngle(interp.prevRotY, interp.nextRotY, t);
			HeadX = Utils.LerpAngle(interp.prev.HeadX, interp.next.HeadX, t);
			
			anim.GetCurrentAnimState(t);
			shouldRender = Model.ShouldRender(this, game.Culling);
			if (shouldRender) Model.Render(this);
		}
		
		public override void RenderName() { 
			if (!shouldRender) return;
			float dist = Model.RenderDistance(this);
			if (dist <= 32 * 32) DrawName();
		}
	}
}