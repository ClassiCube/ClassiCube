// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using ClassicalSharp.Singleplayer;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Entities.Other
{
	/// <summary>
	/// Description of Class1.
	/// </summary>
	public class TntEntity : Entity {
		
		LocalInterpComponent interp;
		CollisionsComponent collisions;
		PhysicsComponent physics;
		static HacksComponent hacks = new HacksComponent(null, null);
		
		short fuse = 80;
		short timer = 0;
		float tntPower = 4;
		
		
		public TntEntity(Game game, short newFuse, float newTntPower) : base(game) {
			interp = new LocalInterpComponent(game, this);
			collisions = new CollisionsComponent(game, this);
			physics = new PhysicsComponent(game, this);
			physics.hacks = hacks;
			physics.collisions = collisions;
			
			this.Velocity = new Vector3(0, 0.2f, -0.02f);
			
			SetModel("46");
			
			if (newFuse >= 0) {
				this.ModelScale += 0.003f * (fuse - newFuse);
				fuse = newFuse;
			}
			
			if (newTntPower >= 0) {
				this.tntPower = newTntPower;
			}
			
			//this.Velocity = new Vector3(0, 0.2f, -0.02f);
		}
		
		public override void RenderName() { }
		
		public override void Despawn() { }
		
		public override void Tick(double delta) {
			if (game.World.IsNotLoaded) return;
			this.OldVelocity = this.Velocity;
			interp.AdvanceState();
			physics.UpdateVelocityState();
			if (timer <= 4 && this.Velocity.Y <= 0) this.Velocity.Y = 0;
			Vector3 vel = new Vector3(0, 0, 0);
			physics.PhysicsTick(vel);
			
			
			timer += 1;
			
			interp.next.Pos = Position; Position = interp.prev.Pos;
			anim.UpdateAnimState(interp.prev.Pos, interp.next.Pos, delta);
			
			fuse -= 1;
			this.ModelScale += 0.003f;
			if (fuse <= 0 ) {
				if (Options.GetBool(OptionsKey.SingleplayerPhysics, true)) {
					int x = (int)this.Position.X;
					int y = (int)this.Position.Y;
					int z = (int)this.Position.Z;
					
					game.tnt.Explode(tntPower, x, y, z);
				}
				game.Entities[this.ID] = null;
			}
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		public override void RenderModel(double deltaTime, float t) {
			Position = Vector3.Lerp(interp.prev.Pos, interp.next.Pos, t);
			interp.LerpAngles(t);
			anim.GetCurrentAnimState(t);
			Model.Render(this);
		}
		
	}
}
