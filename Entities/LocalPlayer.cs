using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class LocalPlayer : Player {
		
		public Vector3 SpawnPoint;
		
		public float ReachDistance = 5f;
		
		public byte UserType;
		bool canSpeed = true, canFly = true, canRespawn = true, canNoclip = true;
		
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
		public float JumpHeight {
			get { return jumpVel == 0 ? 0 : (float)GetMaxHeight( jumpVel ); }
		}
		
		public LocalPlayer( byte id, Game window ) : base( id, window ) {
			DisplayName = window.Username;
			SkinName = window.Username;
			map = window.Map;
		}
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			if( update.IncludesPosition ) {
				nextPos = update.RelativePosition ? nextPos + update.Pos : update.Pos;
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
		
		public override void Despawn() {
			if( renderer != null ) {
				renderer.Dispose();
			}
		}
		
		public override void Tick( double delta ) {
			if( Window.Map.IsNotLoaded ) return;
			
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
			if( renderer != null ) {
				CheckSkin();
			}
		}
		
		public override void Render( double deltaTime, float t ) {
			if( !Window.Camera.IsThirdPerson ) return;
			if( renderer == null ) {
				renderer = new PlayerRenderer( this, Window );
				Window.AsyncDownloader.DownloadSkin( SkinName );
			}
			SetCurrentAnimState( t );
			renderer.Render( deltaTime );
		}
		
		void HandleInput( ref float xMoving, ref float zMoving ) {
			if( Window.ScreenLockedInput ) {
				jumping = speeding = flyingUp = flyingDown = false;
			} else {
				if( Window.IsKeyDown( KeyMapping.Forward ) ) xMoving -= 0.98f;
				if( Window.IsKeyDown( KeyMapping.Back ) ) xMoving += 0.98f;
				if( Window.IsKeyDown( KeyMapping.Left ) ) zMoving -= 0.98f;
				if( Window.IsKeyDown( KeyMapping.Right ) ) zMoving += 0.98f;

				jumping = Window.IsKeyDown( KeyMapping.Jump );
				speeding = CanSpeed && Window.IsKeyDown( KeyMapping.Speed );
				flyingUp = Window.IsKeyDown( KeyMapping.FlyUp );
				flyingDown = Window.IsKeyDown( KeyMapping.FlyDown );
			}
		}
		
		void UpdateVelocityYState() {
			if( flying || noClip ) {
				Velocity.Y = 0; // eliminate the effect of gravity
				float vel = noClip ? 0.24f : 0.06f;
				float velSpeeding = noClip ? 0.48f : 0.08f;
				if( flyingUp || jumping ) {
					Velocity.Y = speeding ? velSpeeding : vel;
				} else if( flyingDown ) {
					Velocity.Y = speeding ? -velSpeeding : -vel;
				}
			} else if( jumping && TouchesAnyRope() && Velocity.Y > 0.02f ) {
				Velocity.Y = 0.02f;
			}

			if( jumping ) {
				if( TouchesAnyWater() || TouchesAnyLava() ) {
					Velocity.Y += speeding ? 0.08f : 0.04f;
				} else if( TouchesAnyRope() ) {
					Velocity.Y += speeding ? 0.15f : 0.10f;
				} else if( onGround ) {
					Velocity.Y = speeding ? jumpVel * 2 : jumpVel;
				}
			}
		}
		
		void AdjHorVelocity( float x, float z, float factor ) {
			float dist = (float)Math.Sqrt( x * x + z * z );
			if( dist < 0.00001f ) return;
			if( dist < 1 ) dist = 1;

			float multiply = factor / dist;
			x *= multiply;
			z *= multiply;
			float cosA = (float)Math.Cos( YawRadians );
			float sinA = (float)Math.Sin( YawRadians );
			Velocity.X += x * cosA - z * sinA;
			Velocity.Z += x * sinA + z * cosA;
		}
		
		void PhysicsTick( float xMoving, float zMoving ) {
			float multiply = flying ? ( speeding ? 90 : 15 ) : ( speeding ? 10 : 1 );

			if( TouchesAnyWater() && !flying && !noClip ) {
				AdjHorVelocity( zMoving * multiply, xMoving * multiply, 0.02f * multiply );
				Move();
				Velocity *= 0.8f;
				Velocity.Y -= 0.02f;
			} else if( TouchesAnyLava() && !flying && !noClip ) {
				AdjHorVelocity( zMoving * multiply, xMoving * multiply, 0.02f * multiply );
				Move();
				Velocity *= 0.5f;
				Velocity.Y -= 0.02f; // gravity
			} else if( TouchesAnyRope() && !flying && !noClip ) {
				multiply = 1.7f;
				AdjHorVelocity( zMoving, xMoving, 0.02f * multiply );
				Move();
				Velocity *= 0.5f;
				Velocity.Y = ( Velocity.Y - 0.02f ) * multiply; // gravity
			} else {
				float factor = !flying && onGround ? 0.1f : 0.02f;
				AdjHorVelocity( zMoving, xMoving, factor * multiply );
				multiply /= 5;
				if( multiply < 1 ) multiply = 1;
				
				Velocity.Y *= multiply;
				Move();
				Velocity.Y /= multiply;
				// Apply general drag and gravity
				Velocity.X *= 0.91f;
				Velocity.Y *= 0.98f;
				Velocity.Z *= 0.91f;
				Velocity.Y -= 0.08f;
				
				if( BlockUnderFeet == Block.Ice ) {
					Utils.Clamp( ref Velocity.X, -0.25f, 0.25f );
					Utils.Clamp( ref Velocity.Z, -0.25f, 0.25f );
				} else if( onGround || flying ) {
					// Apply air resistance or ground friction
					Velocity.X *= 0.6f;
					Velocity.Z *= 0.6f;
				}
			}
		}
		
		void Move() {
			if( !noClip ) {
				MoveAndWallSlide();
			}
			Position += Velocity;
		}		
		
		bool jumping, speeding, flying, noClip, flyingDown, flyingUp;
		public void ParseHackFlags( string name, string motd ) {
			string joined = name + motd;
			if( joined.Contains( "-hax" ) ) {
				CanFly = CanNoclip = CanSpeed = CanRespawn = false;
				Window.CanUseThirdPersonCamera = false;
				Window.SetCamera( false );
			} else { // By default (this is also the case with WoM), we can use hacks.
				CanFly = CanNoclip = CanSpeed = CanRespawn = true;
				Window.CanUseThirdPersonCamera = true;
			}

			// Determine if specific hacks are or are not allowed.
			if( joined.Contains( "+fly" ) ) {
				CanFly = true;
			} else if( joined.Contains( "-fly" ) ) {
				CanFly = false;
			}
			if( joined.Contains( "+noclip" ) ) {
				CanNoclip = true;
			} else if( joined.Contains( "-noclip" ) ) {
				CanNoclip = false;
			}
			if( joined.Contains( "+speed" ) ) {
				CanSpeed = true;
			} else if( joined.Contains( "-speed" ) ) {
				CanSpeed = false;
			}
			if( joined.Contains( "+respawn" ) ) {
				CanRespawn = true;
			} else if( joined.Contains( "-respawn" ) ) {
				CanRespawn = false;
			}
			
			// Operator override.
			if( UserType == 0x64 ) {
				if( joined.Contains( "+ophax" ) ) {
					CanFly = CanNoclip = CanSpeed = CanRespawn = true;
				} else if( joined.Contains( "-ophax" ) ) {
					CanFly = CanNoclip = CanSpeed = CanRespawn = false;
				}
			}
		}
		
		Vector3 lastPos, nextPos;
		float lastYaw, nextYaw, lastPitch, nextPitch;
		public void SetInterpPosition( float t ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			YawDegrees = Utils.Lerp( lastYaw, nextYaw, t );
			PitchDegrees = Utils.Lerp( lastPitch, nextPitch, t );
		}
		
		public bool HandleKeyDown( Key key ) {
			if( key == Window.Keys[KeyMapping.Respawn] && canRespawn ) {
				LocationUpdate update = LocationUpdate.MakePos( SpawnPoint, false );
				SetLocation( update, false );
			} else if( key == Window.Keys[KeyMapping.SetSpawn] && canRespawn ) {
				SpawnPoint = Position;
			} else if( key == Window.Keys[KeyMapping.Fly] && canFly ) {
				flying = !flying;
			} else if( key == Window.Keys[KeyMapping.NoClip] && canNoclip ) {
				noClip = !noClip;
			} else {
				return false;
			}
			return true;
		}
		
		internal void CalculateJumpVelocity( float jumpHeight ) {
			if( jumpHeight == 0 ) {
				jumpVel = 0;
				return;
			}

			float jumpV = 0.01f;
			if( jumpHeight >= 256 ) jumpV = 10.0f;
			if( jumpHeight >= 512 ) jumpV = 16.5f;
			if( jumpHeight >= 768 ) jumpV = 22.5f;
			
			while( GetMaxHeight( jumpV ) <= jumpHeight ) {
				jumpV += 0.01f;
			}
			jumpVel = jumpV;
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
			// e^(-0.0202027 t) * (-49u - 196) - 4t + 50u + 196
			// which is the same as (0.98^t) * (-49u - 196) - 4t + 50u + 196
			double a = Math.Exp( -0.0202027 * t ); //~0.98^t
			return a * ( -49 * u - 196 ) - 4 * t + 50 * u + 196;
		}
	}
}