using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class LocalPlayer : Player {
		
		public Vector3 SpawnPoint;
		public short Health;
		
		public float ReachDistance = 5f;
		
		bool canSpeed = true, canFly = true, canNoclip = true;
		
		public bool CanSpeed {
			get { return canSpeed; }
			set { canSpeed = value; }
		}
		
		public bool CanFly {
			get { return canFly; }
			set { canFly = value; if( !value ) flying = false; }
		}
		
		public bool CanNoclip {
			get { return canNoclip; }
			set { canNoclip = value; if( !value ) noClip = false; }
		}
		
		public LocalPlayer( Game window ) : base( window ) {
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
		
		void HandleInput( out float xMoving, out float zMoving ) {
			xMoving = 0;
			zMoving = 0;
			if( game.ScreenLockedInput ) {
				jumping = speeding = flyingUp = flyingDown = false;
			} else {
				if( game.IsKeyDown( KeyMapping.Forward ) ) xMoving -= 0.98f;
				if( game.IsKeyDown( KeyMapping.Back ) ) xMoving += 0.98f;
				if( game.IsKeyDown( KeyMapping.Left ) ) zMoving -= 0.98f;
				if( game.IsKeyDown( KeyMapping.Right ) ) zMoving += 0.98f;

				jumping = game.IsKeyDown( KeyMapping.Jump );
				speeding = CanSpeed && game.IsKeyDown( KeyMapping.Speed );
				flyingUp = game.IsKeyDown( KeyMapping.FlyUp );
				flyingDown = game.IsKeyDown( KeyMapping.FlyDown );
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
		
		Vector3 lastPos, nextPos;
		float lastYaw, nextYaw, lastPitch, nextPitch;
		public void SetInterpPosition( float t ) {
			if( t < 0 ) t = 0;
			if( t > 1 ) t = 1;
			
			Position = Vector3.Lerp( lastPos, nextPos, t );
			YawDegrees = Utils.Lerp( lastYaw, nextYaw, t );
			PitchDegrees = Utils.Lerp( lastPitch, nextPitch, t );
		}
		
		int tickCount = 0;
		public override void Tick( double delta ) {
			if( game.Map == null || game.Map.IsNotLoaded ) return;
			map = game.Map;
			
			float xMoving, zMoving;
			lastPos = Position = nextPos;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			HandleInput( out xMoving, out zMoving );
			//UpdateState( xMoving, zMoving );
			//PhysicsTick( xMoving, zMoving );
			HackyPhysics( xMoving, zMoving );
			nextPos = Position;
			Position = lastPos;
			UpdateAnimState( lastPos, nextPos );
			tickCount++;
			if( renderer != null ) {
				Bitmap bmp;
				game.AsyncDownloader.TryGetImage( "skin_" + SkinName, out bmp );
				if( bmp != null ) {
					game.Graphics.DeleteTexture( renderer.TextureId );
					renderer.TextureId = game.Graphics.LoadTexture( bmp );
					SkinType = Utils.GetSkinType( bmp );
					bmp.Dispose();
				}
			}
		}
		
		void HackyPhysics( float xMoving, float zMoving ) {
			float multiply = 1F;
			if( !flying ) {
				multiply = speeding ? 10 : 1;
			} else {
				multiply = speeding ? 90 : 15;
			}
			
			Velocity = Vector3.Zero;
			if( flyingUp || jumping ) {
				Velocity.Y = speeding ? 0.48f : 0.24f;
			} else if( flyingDown ) {
				Velocity.Y = speeding ? -0.48f : -0.24f;
			}
			AdjHorVelocity( zMoving, xMoving, 0.2f * multiply );
			Position += Velocity;
		}
		
		public override void Render( double deltaTime, float t ) {
			if( !game.Camera.IsThirdPerson ) return;
			if( renderer == null ) {
				renderer = new PlayerRenderer( this, game );
				game.AsyncDownloader.DownloadSkin( SkinName );
			}
			SetCurrentAnimState( tickCount, t );
			renderer.Render( deltaTime );
		}
		
		public bool HandleKeyDown( Key key ) {
			if( key == game.Keys[KeyMapping.Fly] && canFly ) {
				flying = !flying;
			} else if( key == game.Keys[KeyMapping.NoClip] && canNoclip ) {
				noClip = !noClip;
			} else {
				return false;
			}
			return true;
		}
	}
}