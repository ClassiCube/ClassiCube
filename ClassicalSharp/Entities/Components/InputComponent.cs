// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Physics;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs input handling. </summary>
	public sealed class InputComponent {
		
		Entity entity;
		Game game;
		public HacksComponent Hacks;
		public PhysicsComponent physics;
		
		public InputComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		internal bool Handles(Key key) {
			LocalPlayer p = (LocalPlayer)entity;
			KeyMap keys = game.Input.Keys;
			
			if (key == keys[KeyBind.Respawn] && Hacks.CanRespawn) {
				DoRespawn();
			} else if (key == keys[KeyBind.SetSpawn] && Hacks.CanRespawn) {
				p.Spawn = entity.Position;
				p.Spawn.X = Utils.Floor(p.Spawn.X) + 0.5f;
				p.Spawn.Z = Utils.Floor(p.Spawn.Z) + 0.5f;
				p.SpawnYaw = entity.YawDegrees;
				p.SpawnPitch = entity.PitchDegrees;				
				DoRespawn();
			} else if (key == keys[KeyBind.Fly] && Hacks.CanFly && Hacks.Enabled) {
				Hacks.Flying = !Hacks.Flying;
			} else if (key == keys[KeyBind.NoClip] && Hacks.CanNoclip && Hacks.Enabled && !Hacks.WOMStyleHacks) {
				if (Hacks.Noclip) entity.Velocity.Y = 0;
				Hacks.Noclip = !Hacks.Noclip;
			} else if (key == keys[KeyBind.Jump] && !entity.onGround && !(Hacks.Flying || Hacks.Noclip)) {
				if (!physics.firstJump && Hacks.CanDoubleJump && Hacks.WOMStyleHacks) {
					physics.DoNormalJump();
					physics.firstJump = true;
				} else if (!physics.secondJump && Hacks.CanDoubleJump && Hacks.WOMStyleHacks) {
					physics.DoNormalJump();
					physics.secondJump = true;
				}
			} else {
				return false;
			}
			return true;
		}
		
		void DoRespawn() {
			LocalPlayer p = (LocalPlayer)entity;
			Vector3 spawn = p.Spawn;
			if (game.World.IsValidPos(Vector3I.Floor(spawn)))
				FindHighestFree(ref spawn);
			
			spawn.Y += 2/16f;
			LocationUpdate update = LocationUpdate.MakePosAndOri(spawn, p.SpawnYaw, p.SpawnPitch, false);
			entity.SetLocation(update, false);
			entity.Velocity = Vector3.Zero;
			
			// Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before
			AABB bb = entity.Bounds;
			bb.Min.Y -= 0.01f; bb.Max.Y = bb.Min.Y;
			entity.onGround = entity.TouchesAny(bb, b => game.BlockInfo.Collide[b] == CollideType.Solid);
		}
		
		void FindHighestFree(ref Vector3 spawn) {
			BlockInfo info = game.BlockInfo;
			AABB bb = AABB.Make(spawn, entity.Size);
			
			Vector3I P = Vector3I.Floor(spawn);
			// Spawn player at highest valid position.
			for (int y = P.Y; y <= game.World.Height; y++) {
				float spawnY = Respawn.HighestFreeY(game, ref bb);
				if (spawnY == float.NegativeInfinity) {
					byte block = game.World.GetPhysicsBlock(P.X, y, P.Z);
					float height = info.Collide[block] == CollideType.Solid ? info.MaxBB[block].Y : 0;
					spawn.Y = y + height + Entity.Adjustment;
					return;
				}
				bb.Min.Y += 1; bb.Max.Y += 1;
			}
		}
	}
}