using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
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
		
		public LocalPlayer( byte id, Game window ) : base( id, window ) {
			DisplayName = window.Username;
			SkinName = window.Username;
		}
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			if( update.IncludesPosition ) {
				nextPos = update.RelativePosition ? nextPos + update.Pos : update.Pos;
			}
			if( update.IncludesOrientation ) {
				nextYaw = update.Yaw;
				nextPitch = update.Pitch;
			}
			if( !interpolate ) {
				if( update.IncludesPosition ) {
					lastPos = Position = nextPos;
				}
				if( update.IncludesOrientation ) {
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
		
		static float Sin( double degrees ) {
			return (float)Math.Sin( degrees * Math.PI / 180.0 );
		}
		
		static float Cos( double degrees ) {
			return (float)Math.Cos( degrees * Math.PI / 180.0 );
		}

		bool jumping, speeding, flying, noClip, flyingDown, flyingUp;
		float jumpVelocity = 0.42f;
		
		
		public float JumpHeight {
			get { return jumpVelocity == 0 ? 0 : (float)GetMaxHeight( jumpVelocity ); }
		}
		
		public void CalculateJumpVelocity( float jumpHeight ) {
			if( jumpHeight == 0 ) {
				jumpVelocity = 0;
				return;
			}

			float jumpV = 0.01f;
			if( jumpHeight >= 256 ) jumpV = 10.0f;
			if( jumpHeight >= 512 ) jumpV = 16.5f;
			if( jumpHeight >= 768 ) jumpV = 22.5f;
			
			while( GetMaxHeight( jumpV ) <= jumpHeight ) {
				jumpV += 0.01f;
			}
			jumpVelocity = jumpV;
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
		
		void HandleInput( out float xMoving, out float zMoving ) {
			xMoving = 0;
			zMoving = 0;
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
		
		void UpdateState( float xMoving, float zMoving ) {
			if( flying || noClip ) {
				Velocity.Y = 0; // counter the effect of gravity
			}
			if( noClip ) {
				if( flyingUp || jumping ) {
					Velocity.Y = speeding ? 0.48f : 0.24f;
				} else if( flyingDown ) {
					Velocity.Y = speeding ? -0.48f : -0.24f;
				}
			} else if( flying ) {
				if( flyingUp || jumping ) {
					Velocity.Y = speeding ? 0.08f : 0.06f;
				} else if( flyingDown ) {
					Velocity.Y = speeding ? -0.08f : -0.06f;
				}
			} else if( !noClip && !flyingDown && jumping && TouchesAnyRope() && Velocity.Y > 0.02f ) {
				Velocity.Y = 0.02f;
			}

			if( jumping ) {
				if( TouchesAnyWater() ) {
					Velocity.Y += speeding ? 0.08f : 0.04f;
				} else if( TouchesAnyLava() ) {
					Velocity.Y += speeding ? 0.08f : 0.04f;
				} else if( TouchesAnyRope() ) {
					Velocity.Y += speeding ? 0.15f : 0.10f;
				} else if( onGround ) {
					Velocity.Y = speeding ? jumpVelocity * 2 : jumpVelocity;
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
			float xDir = Cos( YawDegrees );
			float yDir = Sin( YawDegrees );
			Velocity.X += x * xDir - z * yDir;
			Velocity.Z += x * yDir + z * xDir;
		}
		
		void PhysicsTick( float xMoving, float zMoving ) {
			float multiply = 1F;
			if( !flying ) {
				multiply = speeding ? 10 : 1;
			} else {
				multiply = speeding ? 90 : 15;
			}

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
				if( !flying ) {
					AdjHorVelocity( zMoving, xMoving, ( onGround ? 0.1f : 0.02f ) * multiply );
				} else {
					AdjHorVelocity( zMoving, xMoving, 0.02f * multiply );
				}
				multiply /= 5;
				if( multiply < 1 ) multiply = 1;
				
				Velocity.Y *= multiply;
				Move();
				Velocity.Y /= multiply;
				Vector3I blockCoords = Vector3I.Floor( Position.X , Position.Y - 0.01f, Position.Z );
				byte blockUnder = GetPhysicsBlockId( blockCoords.X, blockCoords.Y, blockCoords.Z );

				// Apply general drag
				Velocity.X *= 0.91f;
				Velocity.Y *= 0.98f;
				Velocity.Z *= 0.91f;
				Velocity.Y -= 0.08f;
				
				if( blockUnder == (byte)Block.Ice ) {
					// Limit velocity while travelling on ice.
					if( Velocity.X > 0.25f ) Velocity.X = 0.25f;
					if( Velocity.X < -0.25f ) Velocity.X = -0.25f;
					if( Velocity.Z > 0.25f ) Velocity.Z = 0.25f;
					if( Velocity.Z < -0.25f ) Velocity.Z = -0.25f;
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
		
		public void ParseHackFlags( string name, string motd ) {
			if( name.Contains( "-hax" ) || motd.Contains( "-hax" ) ) {
				CanFly = CanNoclip = CanSpeed = CanRespawn = false;
				Window.CanUseThirdPersonCamera = false;
				Window.SetCamera( false );
			} else { // By default (this is also the case with WoM), we can use hacks.
				CanFly = CanNoclip = CanSpeed = CanRespawn = true;
				Window.CanUseThirdPersonCamera = true;
			}

			// Determine if specific hacks are or are not allowed.
			if( name.Contains( "+fly" ) || motd.Contains( "+fly" ) ) {
				CanFly = true;
			} else if( name.Contains( "-fly" ) || motd.Contains( "-fly" ) ) {
				CanFly = false;
			}
			if( name.Contains( "+noclip" ) || motd.Contains( "+noclip" ) ) {
				CanNoclip = true;
			} else if( name.Contains( "-noclip" ) || motd.Contains( "-noclip" ) ) {
				CanNoclip = false;
			}
			if( name.Contains( "+speed" ) || motd.Contains( "+speed" ) ) {
				CanSpeed = true;
			} else if( name.Contains( "-speed" ) || motd.Contains( "-speed" ) ) {
				CanSpeed = false;
			}
			if( name.Contains( "+respawn" ) || motd.Contains( "+respawn" ) ) {
				CanRespawn = true;
			} else if( name.Contains( "-respawn" ) || motd.Contains( "-respawn" ) ) {
				CanRespawn = false;
			}
			
			// Operator override.
			if( UserType == 0x64 ) {
				if( name.Contains( "+ophax" ) || motd.Contains( "+ophax" ) ) {
					CanFly = CanNoclip = CanSpeed = CanRespawn = true;
				} else if( name.Contains( "-ophax" ) || motd.Contains( "-ophax" ) ) {
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
		
		int tickCount = 0;
		public override void Tick( double delta ) {
			if( Window.Map == null || Window.Map.IsNotLoaded ) return;
			map = Window.Map;
			
			float xMoving, zMoving;
			lastPos = Position = nextPos;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			HandleInput( out xMoving, out zMoving );
			UpdateState( xMoving, zMoving );
			PhysicsTick( xMoving, zMoving );
			nextPos = Position;
			Position = lastPos;
			UpdateAnimState( lastPos, nextPos, delta );
			tickCount++;
			if( renderer != null ) {
				Bitmap bmp;
				Window.AsyncDownloader.TryGetImage( "skin_" + SkinName, out bmp );
				if( bmp != null ) {
					Window.Graphics.DeleteTexture( renderer.TextureId );
					renderer.TextureId = Window.Graphics.LoadTexture( bmp );
					SkinType = Utils.GetSkinType( bmp );
					bmp.Dispose();
				}
			}
		}
		
		public override void Render( double deltaTime, float t, StandardShader shader ) {
			if( !Window.Camera.IsThirdPerson ) return;
			if( renderer == null ) {
				renderer = new PlayerRenderer( this, Window );
				Window.AsyncDownloader.DownloadSkin( SkinName );
			}
			SetCurrentAnimState( t );
			renderer.Render( deltaTime, shader );
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
	}
}