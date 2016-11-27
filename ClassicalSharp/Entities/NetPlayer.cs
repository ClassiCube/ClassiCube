// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public sealed class NetPlayer : Player {
		
		InterpolatedComponent interp;
		public NetPlayer(string displayName, string skinName, Game game, byte id) : base(game) {
			DisplayName = displayName;
			SkinName = skinName;
			SkinIdentifier = "skin_" + id;
			interp = new InterpolatedComponent(game, this);
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		public override void Tick(double delta) {
			CheckSkin();
			tickCount++;
			interp.UpdateCurrentState();
			anim.UpdateAnimState(interp.oldState.pos, interp.newState.pos, delta);
		}

		bool shouldRender = false;
		public override void RenderModel(double deltaTime, float t) {
			Position = Vector3.Lerp(interp.oldState.pos, interp.newState.pos, t);
			HeadYawDegrees = Utils.LerpAngle(interp.oldState.headYaw, interp.newState.headYaw, t);
			YawDegrees = Utils.LerpAngle(interp.oldYaw, interp.newYaw, t);
			PitchDegrees = Utils.LerpAngle(interp.oldState.pitch, interp.newState.pitch, t);
			
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