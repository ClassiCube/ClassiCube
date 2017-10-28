// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using ClassicalSharp.Physics;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs collision detection. </summary>
	public sealed class PhysicsComponent {
		
		bool useLiquidGravity = false; // used by BlockDefinitions.
		bool canLiquidJump = true;
		internal bool jumping;
		internal int multiJumps;
		Entity entity;
		Game game;
		
		internal float jumpVel = 0.42f, userJumpVel = 0.42f, serverJumpVel = 0.42f;
		internal HacksComponent hacks;
		internal CollisionsComponent collisions;
		
		public PhysicsComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;
		}
		
		public void UpdateVelocityState() {
			if (hacks.Floating) {
				entity.Velocity.Y = 0; // eliminate the effect of gravity
				int dir = (hacks.FlyingUp || jumping) ? 1 : (hacks.FlyingDown ? -1 : 0);
				
				entity.Velocity.Y += 0.12f * dir;
				if (hacks.Speeding && hacks.CanSpeed) entity.Velocity.Y += 0.12f * dir;
				if (hacks.HalfSpeeding && hacks.CanSpeed) entity.Velocity.Y += 0.06f * dir;
			} else if (jumping && entity.TouchesAnyRope() && entity.Velocity.Y > 0.02f) {
				entity.Velocity.Y = 0.02f;
			}
			
			if (!jumping) {
				canLiquidJump = false; return;
			}
			
			bool touchWater = entity.TouchesAnyWater();
			bool touchLava = entity.TouchesAnyLava();
			if (touchWater || touchLava) {
				AABB bounds = entity.Bounds;
				int feetY = Utils.Floor(bounds.Min.Y), bodyY = feetY + 1;
				int headY = Utils.Floor(bounds.Max.Y);
				if (bodyY > headY) bodyY = headY;
				
				bounds.Max.Y = bounds.Min.Y = feetY;
				bool liquidFeet = entity.TouchesAny(bounds, touchesLiquid);
				bounds.Min.Y = Math.Min(bodyY, headY);
				bounds.Max.Y = Math.Max(bodyY, headY);
				bool liquidRest = entity.TouchesAny(bounds, touchesLiquid);
				
				bool pastJumpPoint = liquidFeet && !liquidRest && (entity.Position.Y % 1 >= 0.4);
				if (!pastJumpPoint) {
					canLiquidJump = true;
					entity.Velocity.Y += 0.04f;
					if (hacks.Speeding && hacks.CanSpeed) entity.Velocity.Y += 0.04f;
					if (hacks.HalfSpeeding && hacks.CanSpeed) entity.Velocity.Y += 0.02f;
				} else if (pastJumpPoint) {
					// either A) jump bob in water B) climb up solid on side
					if (collisions.HorizontalCollision)
						entity.Velocity.Y += touchLava ? 0.30f : 0.13f;
					else if (canLiquidJump)
						entity.Velocity.Y += touchLava ? 0.20f : 0.10f;
					canLiquidJump = false;
				}
			} else if (useLiquidGravity) {
				entity.Velocity.Y += 0.04f;
				if (hacks.Speeding && hacks.CanSpeed) entity.Velocity.Y += 0.04f;
				if (hacks.HalfSpeeding && hacks.CanSpeed) entity.Velocity.Y += 0.02f;
				canLiquidJump = false;
			} else if (entity.TouchesAnyRope()) {
				entity.Velocity.Y += (hacks.Speeding && hacks.CanSpeed) ? 0.15f : 0.10f;
				canLiquidJump = false;
			} else if (entity.onGround) {
				DoNormalJump();
			}
		}
		
		public void DoNormalJump() {
			if (jumpVel == 0 || hacks.MaxJumps == 0) return;
			
			entity.Velocity.Y = jumpVel;
			if (hacks.Speeding && hacks.CanSpeed) entity.Velocity.Y += jumpVel;
			if (hacks.HalfSpeeding && hacks.CanSpeed) entity.Velocity.Y += jumpVel / 2;
			canLiquidJump = false;
		}
		
		static Predicate<BlockID> touchesLiquid = IsLiquidCollide;
		static bool IsLiquidCollide(BlockID block) { return BlockInfo.Collide[block] == CollideType.Liquid; }
		
		static Vector3 waterDrag = new Vector3(0.8f, 0.8f, 0.8f),
		lavaDrag = new Vector3(0.5f, 0.5f, 0.5f),
		ropeDrag = new Vector3(0.5f, 0.85f, 0.5f);
		const float liquidGrav = 0.02f, ropeGrav = 0.034f;
		
		public void PhysicsTick(Vector3 vel) {
			if (hacks.Noclip) entity.onGround = false;
			float baseSpeed = GetBaseSpeed();
			float verSpeed = baseSpeed * Math.Max(1, GetSpeed(hacks.CanSpeed, 8f) / 5);
			float horSpeed = baseSpeed * GetSpeed(true, 8f/5);
			// previously horSpeed used to be multiplied by factor of 0.02 in last case
			// it's now multiplied by 0.1, so need to divide by 5 so user speed modifier comes out same
			
			bool womSpeedBoost = hacks.CanDoubleJump && hacks.WOMStyleHacks;
			if (!hacks.Floating && womSpeedBoost) {
				if (multiJumps == 1) { horSpeed *= 46.5f; verSpeed *= 7.5f; }
				else if (multiJumps > 1) { horSpeed *= 93f; verSpeed *= 10f; }
			}
			
			if (entity.TouchesAnyWater() && !hacks.Floating) {
				MoveNormal(vel, 0.02f * horSpeed, waterDrag, liquidGrav, verSpeed);
			} else if (entity.TouchesAnyLava() && !hacks.Floating) {
				MoveNormal(vel, 0.02f * horSpeed, lavaDrag, liquidGrav, verSpeed);
			} else if (entity.TouchesAnyRope() && !hacks.Floating) {
				MoveNormal(vel, 0.02f * 1.7f, ropeDrag, ropeGrav, verSpeed);
			} else {
				float factor = hacks.Floating || entity.onGround ? 0.1f : 0.02f;
				float gravity = useLiquidGravity ? liquidGrav : entity.Model.Gravity;
				
				if (hacks.Floating) {
					MoveFlying(vel, factor * horSpeed, entity.Model.Drag, gravity, verSpeed);
				} else {
					MoveNormal(vel, factor * horSpeed, entity.Model.Drag, gravity, verSpeed);
				}

				if (OnIce(entity) && !hacks.Floating) {
					// limit components to +-0.25f by rescaling vector to [-0.25, 0.25]
					if (Math.Abs(entity.Velocity.X) > 0.25f || Math.Abs(entity.Velocity.Z) > 0.25f) {
						float scale = Math.Min(
							Math.Abs(0.25f / entity.Velocity.X), Math.Abs(0.25f / entity.Velocity.Z));
						entity.Velocity.X *= scale;
						entity.Velocity.Z *= scale;
					}
				} else if (entity.onGround || hacks.Flying) {
					entity.Velocity = Utils.Mul(entity.Velocity, entity.Model.GroundFriction); // air drag or ground friction
				}
			}
			
			if (entity.onGround) multiJumps = 0;
		}
		
		
		bool OnIce(Entity entity) {
			Vector3 under = entity.Position; under.Y -= 0.01f;
			if (BlockInfo.ExtendedCollide[GetBlock(under)] == CollideType.Ice) return true;
			
			AABB bounds = entity.Bounds;
			bounds.Min.Y -= 0.01f; bounds.Max.Y = bounds.Min.Y;
			return entity.TouchesAny(bounds, touchesSlipperyIce);
		}
		
		BlockID GetBlock(Vector3 coords) {
			return game.World.SafeGetBlock(Vector3I.Floor(coords));
		}
		
		static Predicate<BlockID> touchesSlipperyIce = IsSlipperyIce;
		static bool IsSlipperyIce(BlockID b) { return BlockInfo.ExtendedCollide[b] == CollideType.SlipperyIce; }
		
		void MoveHor(Vector3 vel, float factor) {
			float dist = (float)Math.Sqrt(vel.X * vel.X + vel.Z * vel.Z);
			if (dist < 0.00001f) return;
			if (dist < 1) dist = 1;
			
			entity.Velocity += vel * (factor / dist);
		}
		
		void MoveFlying(Vector3 vel, float factor, Vector3 drag, float gravity, float yMul) {
			MoveHor(vel, factor);
			float yVel = (float)Math.Sqrt(entity.Velocity.X * entity.Velocity.X + entity.Velocity.Z * entity.Velocity.Z);
			// make horizontal speed the same as vertical speed.
			if ((vel.X != 0 || vel.Z != 0) && yVel > 0.001f) {
				entity.Velocity.Y = 0;
				yMul = 1;
				if (hacks.FlyingUp || jumping) entity.Velocity.Y += yVel;
				if (hacks.FlyingDown) entity.Velocity.Y -= yVel;
			}
			Move(drag, gravity, yMul);
		}
		
		void MoveNormal(Vector3 vel, float factor, Vector3 drag, float gravity, float yMul) {
			MoveHor(vel, factor);
			Move(drag, gravity, yMul);
		}
		
		void Move(Vector3 drag, float gravity, float yMul) {
			entity.Velocity.Y *= yMul;
			if (!hacks.Noclip)
				collisions.MoveAndWallSlide();
			entity.Position += entity.Velocity;
			
			entity.Velocity.Y /= yMul;
			entity.Velocity = Utils.Mul(entity.Velocity, drag);
			entity.Velocity.Y -= gravity;
		}

		float GetSpeed(bool canSpeed, float speedMul) {
			float factor = hacks.Floating ? speedMul : 1, speed = factor;		
			if (hacks.Speeding && canSpeed) speed += factor * hacks.SpeedMultiplier;
			if (hacks.HalfSpeeding && canSpeed) speed += factor * hacks.SpeedMultiplier / 2;
			return hacks.CanSpeed ? speed : Math.Min(speed, hacks.MaxSpeedMultiplier);
		}
		
		const float inf = float.PositiveInfinity;
		float GetBaseSpeed() {
			AABB bounds = entity.Bounds;
			useLiquidGravity = false;
			float baseModifier = LowestModifier(bounds, false);
			bounds.Min.Y -= 0.5f/16f; // also check block standing on
			float solidModifier = LowestModifier(bounds, true);
			
			if (baseModifier == inf && solidModifier == inf) return 1;
			return baseModifier == inf ? solidModifier : baseModifier;
		}
		
		float LowestModifier(AABB bounds, bool checkSolid) {
			Vector3I min = Vector3I.Floor(bounds.Min);
			Vector3I max = Vector3I.Floor(bounds.Max);
			float modifier = inf;
			
			AABB blockBB = default(AABB);
			min.X = min.X < 0 ? 0 : min.X; max.X = max.X > game.World.MaxX ? game.World.MaxX : max.X;
			min.Y = min.Y < 0 ? 0 : min.Y; max.Y = max.Y > game.World.MaxY ? game.World.MaxY : max.Y;
			min.Z = min.Z < 0 ? 0 : min.Z; max.Z = max.Z > game.World.MaxZ ? game.World.MaxZ : max.Z;
			
			for (int y = min.Y; y <= max.Y; y++)
				for (int z = min.Z; z <= max.Z; z++)
					for (int x = min.X; x <= max.X; x++)
			{
				BlockID block = game.World.GetBlock(x, y, z);
				if (block == 0) continue;
				byte collide = BlockInfo.Collide[block];
				if (collide == CollideType.Solid && !checkSolid) continue;
				
				Vector3 v = new Vector3(x, y, z);
				blockBB.Min = v + BlockInfo.MinBB[block];
				blockBB.Max = v + BlockInfo.MaxBB[block];
				if (!blockBB.Intersects(bounds)) continue;
				
				modifier = Math.Min(modifier, BlockInfo.SpeedMultiplier[block]);
				if (BlockInfo.ExtendedCollide[block] == CollideType.Liquid)
					useLiquidGravity = true;
			}
			return modifier;
		}
		
		/// <summary> Calculates the jump velocity required such that when a client presses
		/// the jump binding they will be able to jump up to the given height. </summary>
		internal void CalculateJumpVelocity(bool userVel, float jumpHeight) {
			jumpVel = 0;
			if (jumpHeight == 0) return;
			
			if (jumpHeight >= 256) jumpVel = 10.0f;
			if (jumpHeight >= 512) jumpVel = 16.5f;
			if (jumpHeight >= 768) jumpVel = 22.5f;
			
			while (GetMaxHeight(jumpVel) <= jumpHeight) {
				jumpVel += 0.001f;
			}
			if (userVel) userJumpVel = jumpVel;
		}
		
		public static double GetMaxHeight(float u) {
			// equation below comes from solving diff(x(t, u))= 0
			// We only work in discrete timesteps, so test both rounded up and down.
			double t = 49.49831645 * Math.Log(0.247483075 * u + 0.9899323);
			return Math.Max(YPosAt((int)t, u), YPosAt((int)t + 1, u));
		}
		
		static double YPosAt(int t, float u) {
			// v(t, u) = (4 + u) * (0.98^t) - 4, where u = initial velocity
			// x(t, u) = Σv(t, u) from 0 to t (since we work in discrete timesteps)
			// plugging into Wolfram Alpha gives 1 equation as
			// (0.98^t) * (-49u - 196) - 4t + 50u + 196
			double a = Math.Exp(-0.0202027 * t); //~0.98^t
			return a * (-49 * u - 196) - 4 * t + 50 * u + 196;
		}
		
		public void DoEntityPush() {
			for (int id = 0; id < EntityList.MaxCount; id++) {
				Entity other = game.Entities.List[id];
				if (other == null || other == entity) continue;
				if (!other.Model.Pushes) continue;
				
				bool yIntersects = 
					entity.Position.Y <= (other.Position.Y + other.Size.Y) && 
					other.Position.Y  <= (entity.Position.Y + entity.Size.Y);
				if (!yIntersects) continue;
				
				float dX = other.Position.X - entity.Position.X;
				float dZ = other.Position.Z - entity.Position.Z;
				float dist = dX * dX + dZ * dZ;
				if (dist < 0.0001f || dist > 1f) continue; // TODO: range needs to be lower?
				
				Vector3 dir = Vector3.Normalize(dX, 0, dZ);
				entity.Velocity -= dir * (1 - dist) / 32f; // TODO: should be 24/25
			}
		}
	}
}