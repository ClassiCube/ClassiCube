// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Physics;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Entities {
	
	/// <summary>
	/// Entity component that performs collision detection.
	/// </summary>
	public sealed class PhysicsComponent {

		readonly Game game;
		Entity entity;

		readonly BlockInfo info;

		bool useLiquidGravity = false; // Used by BlockDefinitions.
		bool canLiquidJump = true;
		internal bool firstJump, secondJump, jumping;

		internal float jumpVel = 0.42f, userJumpVel = 0.42f, serverJumpVel = 0.42f;

		internal HacksComponent hacks;
		internal CollisionsComponent collisions;
		
		public PhysicsComponent(Game game, Entity entity) {
			this.game = game;
			this.entity = entity;

			info = this.game.BlockInfo;
		}
		
		public void UpdateVelocityState() {
			if(hacks.Flying || hacks.Noclip) {
				// Eliminate the effect of gravity.
				entity.Velocity.Y = 0;

				int yDir = (hacks.FlyingUp || jumping) ? 1 : (hacks.FlyingDown ? -1 : 0);
				entity.Velocity.Y += 0.12f * yDir;

				if (hacks.Speeding && hacks.CanSpeed) {
					entity.Velocity.Y += 0.12f * yDir;
				}
				if (hacks.HalfSpeeding && hacks.CanSpeed) {
					entity.Velocity.Y += 0.06f * yDir;
				}
			} else if(jumping && entity.TouchesAnyRope() && entity.Velocity.Y > 0.02f) {
				entity.Velocity.Y = 0.02f;
			}
			
			if (!jumping) {
				canLiquidJump = false;

				return;
			}
			
			bool touchWater = entity.TouchesAnyWater();
			bool touchLava = entity.TouchesAnyLava();

			if(touchWater || touchLava) {
				AABB bounds = entity.Bounds;

				int feetY = Utils.Floor(bounds.Min.Y);
				int bodyY = feetY + 1;
				int headY = Utils.Floor(bounds.Max.Y);

				if (bodyY > headY) {
					bodyY = headY;
				}
				
				bounds.Max.Y = bounds.Min.Y = feetY;
				bounds.Min.Y = Math.Min(bodyY, headY);
				bounds.Max.Y = Math.Max(bodyY, headY);

				bool liquidFeet = entity.TouchesAny(bounds, StandardLiquid);
				bool liquidRest = entity.TouchesAny(bounds, StandardLiquid);
				bool pastJumpPoint = liquidFeet && !liquidRest && (entity.Position.Y % 1 >= 0.4);

				if(!pastJumpPoint) {
					canLiquidJump = true;

					entity.Velocity.Y += 0.04f;

					if(hacks.Speeding && hacks.CanSpeed) {
						entity.Velocity.Y += 0.04f;
					}
					if(hacks.HalfSpeeding && hacks.CanSpeed) {
						entity.Velocity.Y += 0.02f;
					}
				} else if(pastJumpPoint) {
					// Either A) jump bob in water B) climb up solid on side.
					if(collisions.HorizontalCollision) {
						entity.Velocity.Y += touchLava ? 0.30f : 0.13f;
					} else if(canLiquidJump) {
						entity.Velocity.Y += touchLava ? 0.20f : 0.10f;
					}

					canLiquidJump = false;
				}
			} else if(useLiquidGravity) {
				entity.Velocity.Y += 0.04f;

				if (hacks.Speeding && hacks.CanSpeed) {
					entity.Velocity.Y += 0.04f;
				}
				if (hacks.HalfSpeeding && hacks.CanSpeed) {
					entity.Velocity.Y += 0.02f;
				}
				canLiquidJump = false;
			} else if(entity.TouchesAnyRope()) {
				entity.Velocity.Y += (hacks.Speeding && hacks.CanSpeed) ? 0.15f : 0.10f;

				canLiquidJump = false;
			} else if(entity.onGround) {
				DoNormalJump();
			}
		}
		
		public void DoNormalJump() {
			if(jumpVel == 0) {
				return;
			}
			
			entity.Velocity.Y = jumpVel;

			if(hacks.Speeding && hacks.CanSpeed) {
				entity.Velocity.Y += jumpVel;
			}
			if(hacks.HalfSpeeding && hacks.CanSpeed) {
				entity.Velocity.Y += jumpVel / 2;
			}

			canLiquidJump = false;
		}
		
		bool StandardLiquid(BlockID block) {
			return info.Collide[block] == CollideType.SwimThrough;
		}
		
		static Vector3 WaterDrag = new Vector3(0.8f, 0.8f, 0.8f),
		LavaDrag = new Vector3(0.5f, 0.5f, 0.5f),
		RopeDrag = new Vector3(0.5f, 0.85f, 0.5f),
		NormalDrag = new Vector3(0.91f, 0.98f, 0.91f),
		AirDrag = new Vector3(0.6f, 1f, 0.6f);

		const float LiquidGravity = 0.02f,
		RopeGravity = 0.034f,
		NormalGravity = 0.08f;
		
		public void PhysicsTick(Vector3 velocity) {
			if(hacks.Noclip) {
				entity.onGround = false;
			}

			float multiply = GetBaseMultiply(true);
			float yMultiply = GetBaseMultiply(hacks.CanSpeed);
			float modifier = LowestSpeedModifier();

			float yMul = Math.Max(1f, yMultiply / 5) * modifier;
			float horMul = multiply * modifier;

			if(!(hacks.Flying || hacks.Noclip)) {
				if(secondJump) {
					horMul *= 93f;
					yMul *= 10f;
				} else if(firstJump) {
					horMul *= 46.5f;
					yMul *= 7.5f;
				}
			}
			
			if(entity.TouchesAnyWater() && !hacks.Flying && !hacks.Noclip) {
				MoveNormal(velocity, 0.02f * horMul, WaterDrag, LiquidGravity, yMul);
			} else if(entity.TouchesAnyLava() && !hacks.Flying && !hacks.Noclip) {
				MoveNormal(velocity, 0.02f * horMul, LavaDrag, LiquidGravity, yMul);
			} else if(entity.TouchesAnyRope() && !hacks.Flying && !hacks.Noclip) {
				MoveNormal(velocity, 0.02f * 1.7f, RopeDrag, RopeGravity, yMul);
			} else {
				float factor = !(hacks.Flying || hacks.Noclip) && entity.onGround ? 0.1f : 0.02f;
				float gravity = useLiquidGravity ? LiquidGravity : NormalGravity;
				
				if(hacks.Flying || hacks.Noclip) {
					MoveFlying(velocity, factor * horMul, NormalDrag, gravity, yMul);
				} else {
					MoveNormal(velocity, factor * horMul, NormalDrag, gravity, yMul);
				}

				if(entity.BlockUnderFeet == Block.Ice && !(hacks.Flying || hacks.Noclip)) {
					// Limit components to +-0.25f by rescaling vector to [-0.25, 0.25].
					if(Math.Abs(entity.Velocity.X) > 0.25f || Math.Abs(entity.Velocity.Z) > 0.25f) {
						float scale = Math.Min(Math.Abs(0.25f / entity.Velocity.X),
						                       Math.Abs(0.25f / entity.Velocity.Z));

						entity.Velocity.X *= scale;
						entity.Velocity.Z *= scale;
					}
				} else if(entity.onGround || hacks.Flying) {
					// Air drag or ground friction.
					entity.Velocity = Utils.Mul(entity.Velocity, AirDrag);
				}
			}
			
			if(entity.onGround) {
				firstJump = false;
				secondJump = false;
			}
		}
		
		void MoveHor(Vector3 vel, float factor) {
			float dist = (float)Math.Sqrt(vel.X * vel.X + vel.Z * vel.Z);

			if (dist < 0.00001f) {
				return;
			}
			if (dist < 1) {
				dist = 1;
			}
			
			entity.Velocity += vel * (factor / dist);
		}
		
		void MoveFlying(Vector3 velocity, float factor, Vector3 drag, float gravity, float yMul) {
			MoveHor(velocity, factor);

			float yVelocity = (float)Math.Sqrt(entity.Velocity.X * entity.Velocity.X + entity.Velocity.Z * entity.Velocity.Z);

			// Make horizontal speed the same as vertical speed.
			if((velocity.X != 0 || velocity.Z != 0) && yVelocity > 0.001f) {
				entity.Velocity.Y = 0;

				yMul = 1;

				if(hacks.FlyingUp || jumping) {
					entity.Velocity.Y += yVelocity;
				}
				if(hacks.FlyingDown) {
					entity.Velocity.Y -= yVelocity;
				}
			}

			Move(drag, gravity, yMul);
		}
		
		void MoveNormal(Vector3 velocity, float factor, Vector3 drag, float gravity, float yMul) {
			MoveHor(velocity, factor);
			Move(drag, gravity, yMul);
		}
		
		void Move(Vector3 drag, float gravity, float yMul) {
			entity.Velocity.Y *= yMul;

			if(!hacks.Noclip) {
				collisions.MoveAndWallSlide();
			}

			entity.Position += entity.Velocity;
			
			entity.Velocity.Y /= yMul;
			entity.Velocity = Utils.Mul(entity.Velocity, drag);
			entity.Velocity.Y -= gravity;
		}

		float GetBaseMultiply(bool canSpeed) {
			float multiply = 0;
			float speed = (hacks.Flying || hacks.Noclip) ? 8 : 1;
			
			if(hacks.Speeding && canSpeed) {
				multiply += speed * hacks.SpeedMultiplier;
			}
			if(hacks.HalfSpeeding && canSpeed) {
				multiply += speed * hacks.SpeedMultiplier / 2;
			}
			if(multiply == 0) {
				multiply = speed;
			}

			return hacks.CanSpeed ? multiply : Math.Min(multiply, hacks.MaxSpeedMultiplier);
		}
		
		const float Infinity = float.PositiveInfinity;

		float LowestSpeedModifier() {
			AABB bounds = entity.Bounds;
			// Also check block standing on.
			bounds.Min.Y -= 0.5f/16f;

			useLiquidGravity = false;

			float baseModifier = LowestModifier(bounds, false);
			float solidModifier = LowestModifier(bounds, true);
			
			if(baseModifier == Infinity && solidModifier == Infinity) {
				return 1;
			}

			return baseModifier == Infinity ? solidModifier : baseModifier;
		}
		
		float LowestModifier(AABB bounds, bool checkSolid) {
			float modifier = Infinity;

			Vector3I bbMin = Vector3I.Floor(bounds.Min);
			Vector3I bbMax = Vector3I.Floor(bounds.Max);
			
			for(int y = bbMin.Y; y <= bbMax.Y; y++) {
				for(int z = bbMin.Z; z <= bbMax.Z; z++) {
					for(int x = bbMin.X; x <= bbMax.X; x++) {
						BlockID block = game.World.SafeGetBlock(x, y, z);

						if(block == 0) {
							continue;
						}

						CollideType type = info.Collide[block];

						if(type == CollideType.Solid && !checkSolid)
							continue;
						
						Vector3 min = new Vector3(x, y, z) + info.MinBB[block];
						Vector3 max = new Vector3(x, y, z) + info.MaxBB[block];

						AABB blockBB = new AABB(min, max);

						if(!blockBB.Intersects(bounds)) {
							continue;
						}
			
						modifier = Math.Min(modifier, info.SpeedMultiplier[block]);

						if(!info.IsLiquid(block) && type == CollideType.SwimThrough) {
							useLiquidGravity = true;
						}
					}
				}
			}

			return modifier;
		}
		
		/// <summary>
		/// Calculates the jump velocity required such that when a client presses
		/// the jump binding they will be able to jump up to the given height.
		/// </summary>
		internal void CalculateJumpVelocity(bool userVelocity, float jumpHeight) {
			jumpVel = 0;

			if(jumpHeight == 0) {
				return;
			}
			if(jumpHeight >= 256) {
				jumpVel = 10.0f;
			}
			if (jumpHeight >= 512) {
				jumpVel = 16.5f;
			}
			if(jumpHeight >= 768) {
				jumpVel = 22.5f;
			}
			
			while(GetMaxHeight(jumpVel) <= jumpHeight) {
				jumpVel += 0.001f;
			}

			if(userVelocity) {
				userJumpVel = jumpVel;
			}
		}
		
		public static double GetMaxHeight(float u) {
			// Equation below comes from solving diff(x(t, u)) = 0
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
			for(int id = 0; id < EntityList.MaxCount; id++) {
				Entity other = game.Entities[id];

				if(other == null || other == entity) {
					continue;
				}
				
				bool yIntersects = entity.Position.Y <=
								   (other.Position.Y + other.Size.Y) && 
									other.Position.Y <= 
								   (entity.Position.Y + entity.Size.Y);

				if(!yIntersects) {
					continue;
				}
				
				Vector3 distance = other.Position - entity.Position;

				float finalDistance = distance.X * distance.X + distance.Z * distance.Z;

				// TODO: range needs to be lower?
				if(finalDistance < 0.0001f || finalDistance > 1f) {
					continue;
				}
				
				Vector3 direction = Vector3.Normalize(distance.X, 0, distance.Z);

				// TODO: should be 24/25
				entity.Velocity -= direction * (1 - finalDistance) / 32f;
			}
		}
	}
}
