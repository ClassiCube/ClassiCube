// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	public partial class LocalPlayer : Player {
		
		bool useLiquidGravity = false; // used by BlockDefinitions.
		bool canLiquidJump = true;
		bool firstJump = false, secondJump = false;
		
		void UpdateVelocityState( float xMoving, float zMoving ) {
			if( !Hacks.NoclipSlide && (noClip && xMoving == 0 && zMoving == 0) )
				Velocity = Vector3.Zero;			
			if( flying || noClip ) {
				Velocity.Y = 0; // eliminate the effect of gravity
				int dir = (flyingUp || jumping) ? 1 : (flyingDown ? -1 : 0);
				
				Velocity.Y += 0.12f * dir;
				if( speeding && Hacks.CanSpeed ) Velocity.Y += 0.12f * dir;
				if( halfSpeeding && Hacks.CanSpeed ) Velocity.Y += 0.06f * dir;
			} else if( jumping && TouchesAnyRope() && Velocity.Y > 0.02f ) {
				Velocity.Y = 0.02f;
			}
			
			if( !jumping ) {
				canLiquidJump = false; return;
			}
			
			bool touchWater = TouchesAnyWater();
			bool touchLava = TouchesAnyLava();
			if( touchWater || touchLava ) {
				BoundingBox bounds = CollisionBounds;
				int feetY = Utils.Floor( bounds.Min.Y ), bodyY = feetY + 1;
				int headY = Utils.Floor( bounds.Max.Y );
				if( bodyY > headY ) bodyY = headY;
				
				bounds.Max.Y = bounds.Min.Y = feetY;
				bool liquidFeet = TouchesAny( bounds, StandardLiquid );
				bounds.Min.Y = Math.Min( bodyY, headY );
				bounds.Max.Y = Math.Max( bodyY, headY );
				bool liquidRest = TouchesAny( bounds, StandardLiquid );
				
				bool pastJumpPoint = liquidFeet && !liquidRest && (Position.Y % 1 >= 0.4);
				if( !pastJumpPoint ) {
					canLiquidJump = true;
					Velocity.Y += 0.04f;
					if( speeding && Hacks.CanSpeed ) Velocity.Y += 0.04f;
					if( halfSpeeding && Hacks.CanSpeed ) Velocity.Y += 0.02f;
				} else if( pastJumpPoint ) {
					// either A) jump bob in water B) climb up solid on side
					if( canLiquidJump || (physics.collideX || physics.collideZ) )
						Velocity.Y += touchLava ? 0.20f : 0.10f;
					canLiquidJump = false;
				}
			} else if( useLiquidGravity ) {
				Velocity.Y += 0.04f;
				if( speeding && Hacks.CanSpeed ) Velocity.Y += 0.04f;
				if( halfSpeeding && Hacks.CanSpeed ) Velocity.Y += 0.02f;
				canLiquidJump = false;
			} else if( TouchesAnyRope() ) {
				Velocity.Y += (speeding && Hacks.CanSpeed) ? 0.15f : 0.10f;
				canLiquidJump = false;
			} else if( onGround ) {
				DoNormalJump();
			}
		}
		
		void DoNormalJump() {
			Velocity.Y = jumpVel;
			if( speeding && Hacks.CanSpeed ) Velocity.Y += jumpVel;
			if( halfSpeeding && Hacks.CanSpeed ) Velocity.Y += jumpVel / 2;
			canLiquidJump = false;
		}
		
		bool StandardLiquid( byte block ) {
			return info.Collide[block] == CollideType.SwimThrough;
		}
		
		static Vector3 waterDrag = new Vector3( 0.8f, 0.8f, 0.8f ),
		lavaDrag = new Vector3( 0.5f, 0.5f, 0.5f ),
		ropeDrag = new Vector3( 0.5f, 0.85f, 0.5f ),
		normalDrag = new Vector3( 0.91f, 0.98f, 0.91f ),
		airDrag = new Vector3( 0.6f, 1f, 0.6f );
		const float liquidGrav = 0.02f, ropeGrav = 0.034f, normalGrav = 0.08f;
		
		void PhysicsTick( float xMoving, float zMoving ) {
			if( noClip ) onGround = false;
			float multiply = GetBaseMultiply( true );
			float yMultiply = GetBaseMultiply( Hacks.CanSpeed );
			float modifier = LowestSpeedModifier();
			
			float yMul = Math.Max( 1f, yMultiply / 5 ) * modifier;
			float horMul = multiply * modifier;
			if( !(flying || noClip) ) {
				if( secondJump ) { horMul *= 93f; yMul *= 10f; }
				else if( firstJump ) { horMul *= 46.5f; yMul *= 7.5f; }
			}
			
			if( TouchesAnyWater() && !flying && !noClip ) {
				MoveNormal( xMoving, zMoving, 0.02f * horMul, waterDrag, liquidGrav, yMul );
			} else if( TouchesAnyLava() && !flying && !noClip ) {
				MoveNormal( xMoving, zMoving, 0.02f * horMul, lavaDrag, liquidGrav, yMul );
			} else if( TouchesAnyRope() && !flying && !noClip ) {
				MoveNormal( xMoving, zMoving, 0.02f * 1.7f, ropeDrag, ropeGrav, yMul );
			} else {
				float factor = !(flying || noClip) && onGround ? 0.1f : 0.02f;
				float gravity = useLiquidGravity ? liquidGrav : normalGrav;
				if( flying || noClip )
					MoveFlying( xMoving, zMoving, factor * horMul, normalDrag, gravity, yMul );
				else
					MoveNormal( xMoving, zMoving, factor * horMul, normalDrag, gravity, yMul );

				if( BlockUnderFeet == Block.Ice && !(flying || noClip) ) {
					// limit components to +-0.25f by rescaling vector to [-0.25, 0.25]
					if( Math.Abs( Velocity.X ) > 0.25f || Math.Abs( Velocity.Z ) > 0.25f ) {
						float scale = Math.Min(
							Math.Abs( 0.25f / Velocity.X ), Math.Abs( 0.25f / Velocity.Z ) );
						Velocity.X *= scale;
						Velocity.Z *= scale;
					}
				} else if( onGround || flying ) {
					Velocity *= airDrag; // air drag or ground friction
				}
			}
		}
		
		void AdjHeadingVelocity( float x, float z, float factor ) {
			float dist = (float)Math.Sqrt( x * x + z * z );
			if( dist < 0.00001f ) return;
			if( dist < 1 ) dist = 1;

			float multiply = factor / dist;
			Velocity += Utils.RotateY( x * multiply, 0, z * multiply, HeadYawRadians );
		}
		
		void MoveFlying( float xMoving, float zMoving, float factor, Vector3 drag, float gravity, float yMul ) {
			AdjHeadingVelocity( zMoving, xMoving, factor );
			float yVel = (float)Math.Sqrt( Velocity.X * Velocity.X + Velocity.Z * Velocity.Z );
			// make vertical speed the same as vertical speed.
			if( (xMoving != 0 || zMoving != 0) && yVel > 0.001f ) {
				Velocity.Y = 0;
				yMul = 1;
				if( flyingUp || jumping ) Velocity.Y += yVel;
				if( flyingDown ) Velocity.Y -= yVel;
			}
			Move( xMoving, zMoving, factor, drag, gravity, yMul );
		}
		
		void MoveNormal( float xMoving, float zMoving, float factor, Vector3 drag, float gravity, float yMul ) {
			AdjHeadingVelocity( zMoving, xMoving, factor );
			Move( xMoving, zMoving, factor, drag, gravity, yMul );
		}
		
		void Move( float xMoving, float zMoving, float factor, Vector3 drag, float gravity, float yMul ) {
			Velocity.Y *= yMul;
			if( !noClip )
				physics.MoveAndWallSlide();
			Position += Velocity;
			
			Velocity.Y /= yMul;
			Velocity *= drag;
			Velocity.Y -= gravity;
		}

		float GetBaseMultiply( bool canSpeed ) {
			float multiply = 0;
			if( flying || noClip ) {
				if( speeding && canSpeed ) multiply += Hacks.SpeedMultiplier * 8;
				if( halfSpeeding && canSpeed ) multiply += Hacks.SpeedMultiplier * 8 / 2;
				if( multiply == 0 ) multiply = 8f;
			} else {
				if( speeding && canSpeed ) multiply += Hacks.SpeedMultiplier;
				if( halfSpeeding && canSpeed ) multiply += Hacks.SpeedMultiplier / 2;
				if( multiply == 0 ) multiply = 1;
			}
			return Hacks.CanSpeed ? multiply : Math.Min( multiply, Hacks.MaxSpeedMultiplier );
		}
		
		const float inf = float.PositiveInfinity;
		float LowestSpeedModifier() {
			BoundingBox bounds = CollisionBounds;
			useLiquidGravity = false;
			float baseModifier = LowestModifier( bounds, false );
			bounds.Min.Y -= 0.5f/16f; // also check block standing on
			float solidModifier = LowestModifier( bounds, true );
			
			if( baseModifier == inf && solidModifier == inf ) return 1;
			return baseModifier == inf ? solidModifier : baseModifier;
		}
		
		float LowestModifier( BoundingBox bounds, bool checkSolid ) {
			Vector3I bbMin = Vector3I.Floor( bounds.Min );
			Vector3I bbMax = Vector3I.Floor( bounds.Max );
			float modifier = inf;
			
			for( int y = bbMin.Y; y <= bbMax.Y; y++ )
				for( int z = bbMin.Z; z <= bbMax.Z; z++ )
					for( int x = bbMin.X; x <= bbMax.X; x++ )
			{
				byte block = game.World.SafeGetBlock( x, y, z );
				if( block == 0 ) continue;
				CollideType type = info.Collide[block];
				if( type == CollideType.Solid && !checkSolid )
					continue;
				
				Vector3 min = new Vector3( x, y, z ) + info.MinBB[block];
				Vector3 max = new Vector3( x, y, z ) + info.MaxBB[block];
				BoundingBox blockBB = new BoundingBox( min, max );
				if( !blockBB.Intersects( bounds ) ) continue;
				
				modifier = Math.Min( modifier, info.SpeedMultiplier[block] );
				if( block >= BlockInfo.CpeBlocksCount && type == CollideType.SwimThrough )
					useLiquidGravity = true;
			}
			return modifier;
		}
		
		/// <summary> Calculates the jump velocity required such that when a client presses
		/// the jump binding they will be able to jump up to the given height. </summary>
		internal void CalculateJumpVelocity( float jumpHeight ) {
			jumpVel = 0;
			if( jumpHeight >= 256 ) jumpVel = 10.0f;
			if( jumpHeight >= 512 ) jumpVel = 16.5f;
			if( jumpHeight >= 768 ) jumpVel = 22.5f;
			
			while( GetMaxHeight( jumpVel ) <= jumpHeight )
				jumpVel += 0.001f;
		}
		
		static double GetMaxHeight( float u ) {
			// equation below comes from solving diff(x(t, u))= 0
			// We only work in discrete timesteps, so test both rounded up and down.
			double t = 49.49831645 * Math.Log( 0.247483075 * u + 0.9899323 );
			return Math.Max( YPosAt( (int)t, u ), YPosAt( (int)t + 1, u ) );
		}
		
		static double YPosAt( int t, float u ) {
			// v(t, u) = (4 + u) * (0.98^t) - 4, where u = initial velocity
			// x(t, u) = Σv(t, u) from 0 to t (since we work in discrete timesteps)
			// plugging into Wolfram Alpha gives 1 equation as
			// (0.98^t) * (-49u - 196) - 4t + 50u + 196
			double a = Math.Exp( -0.0202027 * t ); //~0.98^t
			return a * ( -49 * u - 196 ) - 4 * t + 50 * u + 196;
		}
	}
}