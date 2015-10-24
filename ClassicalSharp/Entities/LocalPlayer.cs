using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class LocalPlayer : Player {
		
		public Vector3 SpawnPoint;
		
		public float ReachDistance = 5f;
		public int SpeedMultiplier = 10;
		
		public byte UserType;
		bool canSpeed = true, canFly = true, canRespawn = true, canNoclip = true;
		
		/// <summary> Whether the player has permission to increase its speed beyond the normal walking speed. </summary>
		public bool CanSpeed {
			get { return canSpeed; }
			set { canSpeed = value; }
		}
		
		public bool CanFly {
			get { return canFly; }
			set { canFly = value; if( !value ) flying = false; }
		}
		
		public bool CanRespawn {
			get { return canRespawn; }
			set { canRespawn = value; }
		}
		
		public bool CanNoclip {
			get { return canNoclip; }
			set { canNoclip = value; if( !value ) noClip = false; }
		}
		
		float jumpVel = 0.42f;
		/// <summary> Returns the height that the client can currently jump up to.<br/>
		/// Note that when speeding is enabled the client is able to jump much further. </summary>
		public float JumpHeight {
			get { return (float)GetMaxHeight( jumpVel ); }
		}
		
		public LocalPlayer( Game window ) : base( window ) {
			DisplayName = window.Username;
			SkinName = window.Username;
		}
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			if( update.IncludesPosition ) {
				nextPos = update.RelativePosition ? nextPos + update.Pos : update.Pos;
				nextPos.Y += Entity.Adjustment;
				if( !interpolate ) {
					lastPos = Position = nextPos;
				}
			}
			if( update.IncludesOrientation ) {
				nextYaw = update.Yaw;
				nextPitch = update.Pitch;
				if( !interpolate ) {
					lastYaw = YawDegrees = nextYaw;
					lastPitch = PitchDegrees = nextPitch;
				}
			}
		}
		
		public override void Tick( double delta ) {
			if( game.Map.IsNotLoaded ) return;
			
			float xMoving = 0, zMoving = 0;
			lastPos = Position = nextPos;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			HandleInput( ref xMoving, ref zMoving );
			UpdateVelocityYState();
			PhysicsTick( xMoving, zMoving );
			nextPos = Position;
			Position = lastPos;
			UpdateAnimState( lastPos, nextPos, delta );
			if( api != null ) {
				CheckSkin();
			}
		}
		
		public override void Render( double deltaTime, float t ) {
			if( !game.Camera.IsThirdPerson ) return;
			if( api == null ) {
				InitRenderingData();
				game.AsyncDownloader.DownloadSkin( SkinName );
			}
			GetCurrentAnimState( t );
			RenderModel( deltaTime );
		}
		
		void HandleInput( ref float xMoving, ref float zMoving ) {
			if( game.ScreenLockedInput ) {
				jumping = speeding = flyingUp = flyingDown = false;
			} else {
				if( game.IsKeyDown( KeyMapping.Forward ) ) xMoving -= 0.98f;
				if( game.IsKeyDown( KeyMapping.Back ) ) xMoving += 0.98f;
				if( game.IsKeyDown( KeyMapping.Left ) ) zMoving -= 0.98f;
				if( game.IsKeyDown( KeyMapping.Right ) ) zMoving += 0.98f;

				jumping = game.IsKeyDown( KeyMapping.Jump );
				speeding = canSpeed && game.IsKeyDown( KeyMapping.Speed );
				flyingUp = game.IsKeyDown( KeyMapping.FlyUp );
				flyingDown = game.IsKeyDown( KeyMapping.FlyDown );
			}
		}
		
		bool useLiquidGravity = false; // used by BlockDefinitions.
		void UpdateVelocityYState() {
			if( flying || noClip ) {
				Velocity.Y = 0; // eliminate the effect of gravity
				if( flyingUp || jumping ) {
					Velocity.Y = speeding ? 0.24f : 0.12f;
				} else if( flyingDown ) {
					Velocity.Y = speeding ? -0.24f : -0.12f;
				}
			} else if( jumping && TouchesAnyRope() && Velocity.Y > 0.02f ) {
				Velocity.Y = 0.02f;
			}

			if( jumping ) {
				if( TouchesAnyWater() || TouchesAnyLava() ) {
					BoundingBox bounds = CollisionBounds;
					bounds.Min.Y += 1;
					
					bool isSolid = TouchesAny( bounds,
					                         b => info.CollideType[b] != BlockCollideType.WalkThrough || b == (byte)Block.Rope );
					bool pastJumpPoint = Position.Y % 1 >= 0.4;
					if( isSolid || !pastJumpPoint )
						Velocity.Y += speeding ? 0.08f : 0.04f;
					else if( (collideX || collideZ) && !isSolid && pastJumpPoint )
						Velocity.Y += 0.10f;
				} else if( useLiquidGravity ) {
					Velocity.Y += speeding ? 0.08f : 0.04f;
				} else if( TouchesAnyRope() ) {
					Velocity.Y += speeding ? 0.15f : 0.10f;
				} else if( onGround ) {
					Velocity.Y = speeding ? jumpVel * 2 : jumpVel;
				}
			}
		}
		
		static Vector3 waterDrag = new Vector3( 0.8f, 0.8f, 0.8f ),
		lavaDrag = new Vector3( 0.5f, 0.5f, 0.5f ),
		ropeDrag = new Vector3( 0.5f, 0.85f, 0.5f ),
		normalDrag = new Vector3( 0.91f, 0.98f, 0.91f ),
		airDrag = new Vector3( 0.6f, 1f, 0.6f );
		const float liquidGrav = 0.02f, ropeGrav = 0.034f, normalGrav = 0.08f;
		
		void PhysicsTick( float xMoving, float zMoving ) {
			float multiply = (flying || noClip) ?
				(speeding ? SpeedMultiplier * 9 : SpeedMultiplier * 1.5f) :
				(speeding ? SpeedMultiplier : 1);
			float modifier = LowestSpeedModifier();
			float horMul = multiply * modifier;
			float yMul = Math.Max( 1f, multiply / 5 ) * modifier;
			
			if( TouchesAnyWater() && !flying && !noClip ) {
				Move( xMoving, zMoving, 0.02f * horMul, waterDrag, liquidGrav, 1 );
			} else if( TouchesAnyLava() && !flying && !noClip ) {
				Move( xMoving, zMoving, 0.02f * horMul, lavaDrag, liquidGrav, 1 );
			} else if( TouchesAnyRope() && !flying && !noClip ) {
				Move( xMoving, zMoving, 0.02f * 1.7f, ropeDrag, ropeGrav, 1 );
			} else {
				float factor = !(flying || noClip) && onGround ? 0.1f : 0.02f;
				float gravity = useLiquidGravity ? liquidGrav : normalGrav;
				Move( xMoving, zMoving, factor * horMul, normalDrag, gravity, yMul );
				
				if( BlockUnderFeet == Block.Ice ) {
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
			Velocity += Utils.RotateY( x * multiply, 0, z * multiply, YawRadians );
		}
		
		void Move( float xMoving, float zMoving, float factor, Vector3 drag, float gravity, float yMul ) {
			AdjHeadingVelocity( zMoving, xMoving, factor );
			Velocity.Y *= yMul;
			if( !noClip )
				MoveAndWallSlide();
			Position += Velocity;
			
			Velocity.Y /= yMul;
			Velocity *= drag;
			Velocity.Y -= gravity;
		}
		
		internal bool jumping, speeding, flying, noClip, flyingDown, flyingUp;
		
		/// <summary> Parses hack flags specified in the motd and/or name of the server. </summary>
		/// <remarks> Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax </remarks>
		public void ParseHackFlags( string name, string motd ) {
			string joined = name + motd;
			if( joined.Contains( "-hax" ) ) {
				CanFly = CanNoclip = CanSpeed = CanRespawn = false;
				game.CanUseThirdPersonCamera = false;
				game.SetCamera( false );
			} else { // By default (this is also the case with WoM), we can use hacks.
				CanFly = CanNoclip = CanSpeed = CanRespawn = true;
				game.CanUseThirdPersonCamera = true;
			}
			
			ParseFlag( b => CanFly = b, joined, "fly" );
			ParseFlag( b => CanNoclip = b, joined, "noclip" );
			ParseFlag( b => CanSpeed = b, joined, "speed" );
			ParseFlag( b => CanRespawn = b, joined, "respawn" );

			if( UserType == 0x64 )
				ParseFlag( b => CanFly = CanNoclip = CanRespawn = CanSpeed = b, joined, "ophax" );
		}
		
		static void ParseFlag( Action<bool> action, string joined, string flag ) {
			if( joined.Contains( "+" + flag ) ) {
				action( true );
			} else if( joined.Contains( "-" + flag ) ) {
				action( false );
			}
		}
		
		/// <summary> Sets the user type of this user. This is used to control permissions for grass, 
		/// bedrock, water and lava blocks on servers that don't support CPE block permissions. </summary>
		public void SetUserType( byte value ) {
			UserType = value;
			Inventory inv = game.Inventory;
			inv.CanPlace[(int)Block.Bedrock] = value == 0x64;
			inv.CanDelete[(int)Block.Bedrock] = value == 0x64;
			inv.CanPlace[(int)Block.Grass] = value == 0x64;

			inv.CanPlace[(int)Block.Water] = value == 0x64;
			inv.CanPlace[(int)Block.StillWater] = value == 0x64;
			inv.CanPlace[(int)Block.Lava] = value == 0x64;
			inv.CanPlace[(int)Block.StillLava] = value == 0x64;
		}
		
		internal Vector3 lastPos, nextPos;
		internal float lastYaw, nextYaw, lastPitch, nextPitch;
		
		/// <summary> Linearly interpolates position and rotation between the previous and next state. </summary>
		public void SetInterpPosition( float t ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			YawDegrees = Utils.LerpAngle( lastYaw, nextYaw, t );
			PitchDegrees = Utils.LerpAngle( lastPitch, nextPitch, t );
		}
		
		internal void HandleKeyDown( Key key ) {
			KeyMap keys = game.InputHandler.Keys;
			if( key == keys[KeyMapping.Respawn] && canRespawn ) {
				Vector3I p = Vector3I.Floor( SpawnPoint );
				if( game.Map.IsValidPos( p ) ) {
					// Spawn player at highest valid position.
					for( int y = p.Y; y <= game.Map.Height; y++ ) {
						byte block1 = GetPhysicsBlockId( p.X, y, p.Z );
						byte block2 = GetPhysicsBlockId( p.X, y + 1, p.Z );
						if( info.CollideType[block1] != BlockCollideType.Solid &&
						   info.CollideType[block2] != BlockCollideType.Solid ) {
							p.Y = y;
							break;
						}
					}
				}
				Vector3 spawn = (Vector3)p + new Vector3( 0.5f, 0.01f, 0.5f );
				LocationUpdate update = LocationUpdate.MakePos( spawn, false );
				SetLocation( update, false );
			} else if( key == keys[KeyMapping.SetSpawn] && canRespawn ) {
				SpawnPoint = Position;
			} else if( key == keys[KeyMapping.Fly] && canFly ) {
				flying = !flying;
			} else if( key == keys[KeyMapping.NoClip] && canNoclip ) {
				noClip = !noClip;
			}
		}
		
		/// <summary> Calculates the jump velocity required such that when a client presses 
		/// the jump binding they will be able to jump up to the given height. </summary>
		internal void CalculateJumpVelocity( float jumpHeight ) {
			jumpVel = 0;
			if( jumpHeight >= 256 ) jumpVel = 10.0f;
			if( jumpHeight >= 512 ) jumpVel = 16.5f;
			if( jumpHeight >= 768 ) jumpVel = 22.5f;
			
			while( GetMaxHeight( jumpVel ) <= jumpHeight ) {
				jumpVel += 0.01f;
			}
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
		
		float LowestSpeedModifier() {
			BoundingBox bounds = CollisionBounds;
			bounds.Min.Y -= 0.1f; // block standing on
			Vector3I bbMin = Vector3I.Floor( bounds.Min );
			Vector3I bbMax = Vector3I.Floor( bounds.Max );
			float modifier = float.PositiveInfinity;
			useLiquidGravity = false;
			
			for( int x = bbMin.X; x <= bbMax.X; x++ ) {
				for( int y = bbMin.Y; y <= bbMax.Y; y++ ) {
					for( int z = bbMin.Z; z <= bbMax.Z; z++ ) {
						byte block = game.Map.SafeGetBlock( x, y, z );
						if( block == 0 ) continue;
						
						modifier = Math.Min( modifier, info.SpeedMultiplier[block] );
						if( block >= BlockInfo.CpeBlocksCount &&
						   info.CollideType[block] == BlockCollideType.SwimThrough )
							useLiquidGravity = true;
					}
				}
			}
			return modifier == float.PositiveInfinity ? 1 : modifier;
		}
	}
}