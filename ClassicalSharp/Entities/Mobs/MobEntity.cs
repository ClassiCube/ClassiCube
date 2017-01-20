// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

namespace ClassicalSharp.Entities.Mobs {
	
	public class MobEntity : Entity {
		
		LocalInterpComponent interp;
		public MobEntity(Game game, string model) : base(game) {
			StepSize = 0.5f;
			SetModel(model);
			interp = new LocalInterpComponent(game, this);
		}
		
		public override void Despawn() { }
		public override void RenderName() { }
		
		// TODO: this is just so the entities do something, remove later
		static Random rand = new Random();
		public override void Tick(double delta) {
			interp.AdvanceState();
			float inc = rand.Next(0, 10);
			LocationUpdate update = LocationUpdate.MakeOri(interp.nextHeadYaw + inc, 0);
			SetLocation(update, true);
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		public override void RenderModel(double deltaTime, float t) {
			Position = Vector3.Lerp(interp.lastPos, interp.nextPos, t);
			HeadYawDegrees = Utils.LerpAngle(interp.lastHeadYaw, interp.nextHeadYaw, t);
			YawDegrees = Utils.LerpAngle(interp.lastYaw, interp.nextYaw, t);
			PitchDegrees = Utils.LerpAngle(interp.lastPitch, interp.nextPitch, t);
			
			anim.GetCurrentAnimState(t);
			Model.Render(this);
		}
	}
}
