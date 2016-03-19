using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public partial class LocalPlayer : Player {
		
		/// <summary> Position the player's position is set to when the 'respawn' key binding is pressed. </summary>
		public Vector3 SpawnPoint;

		public float SpawnYaw, SpawnPitch;
		
		/// <summary> The distance (in blocks) that players are allowed to
		/// reach to and interact/modify blocks in. </summary>
		public float ReachDistance = 5f;
		
		internal float jumpVel = 0.42f, serverJumpVel = 0.42f;
		/// <summary> Returns the height that the client can currently jump up to.<br/>
		/// Note that when speeding is enabled the client is able to jump much further. </summary>
		public float JumpHeight {
			get { return (float)GetMaxHeight( jumpVel ); }
		}
		
		internal float curWalkTime, curSwing;
		internal PhysicsComponent physics;
		public HacksComponent Hacks;
		
		public LocalPlayer( Game game ) : base( game ) {
			DisplayName = game.Username;
			SkinName = game.Username;
			SkinIdentifier = "skin_255";
			physics = new PhysicsComponent( game, this );
			Hacks = new HacksComponent( game, this );
			
			Hacks.SpeedMultiplier = Options.GetFloat( OptionsKey.Speed, 0.1f, 50, 10 );
			Hacks.PushbackPlacing = Options.GetBool( OptionsKey.PushbackPlacing, false );
			Hacks.NoclipSlide = Options.GetBool( OptionsKey.NoclipSlide, false );
			Hacks.DoubleJump = Options.GetBool( OptionsKey.DoubleJump, false );
			Hacks.Enabled = Options.GetBool( OptionsKey.HacksEnabled, true );
			InitRenderingData();
		}
		
		Vector3 lastSoundPos = new Vector3( float.PositiveInfinity );
		public override void Tick( double delta ) {
			if( game.Map.IsNotLoaded ) return;
			
			float xMoving = 0, zMoving = 0;
			lastPos = Position = nextPos;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			bool wasOnGround = onGround;
			
			HandleInput( ref xMoving, ref zMoving );
			UpdateVelocityState( xMoving, zMoving );
			PhysicsTick( xMoving, zMoving );
			if( onGround ) { firstJump = false; secondJump = false; }
			
			nextPos = Position;
			Position = lastPos;
			anim.UpdateAnimState( lastPos, nextPos, delta );
			CheckSkin();
			
			Vector3 soundPos = nextPos;
			GetSound();
			if( !anyNonAir ) soundPos = new Vector3( -100000 );
			
			if( onGround && (DoPlaySound( soundPos ) || !wasOnGround) ) {
				game.AudioPlayer.PlayStepSound( sndType );
				lastSoundPos = soundPos;
			}
			UpdateCurrentBodyYaw();
		}
		
		bool DoPlaySound( Vector3 soundPos ) {
			float distSq = (lastSoundPos - soundPos).LengthSquared;
			bool enoughDist = distSq > 1.75f * 1.75f;
			// just play every certain block interval when not animating
			if( curSwing < 0.999f ) return enoughDist;
			
			// have our legs just crossed over the '0' point?
			float oldLegRot = (float)Math.Sin( anim.walkTimeO );
			float newLegRot = (float)Math.Sin( anim.walkTimeN );
			return Math.Sign( oldLegRot ) != Math.Sign( newLegRot );
		}
		
		bool anyNonAir = false;
		SoundType sndType = SoundType.None;
		void GetSound() {
			Vector3 pos = nextPos, size = CollisionSize;
			BoundingBox bounds = new BoundingBox( pos - size / 2, pos + size / 2 );
			sndType = SoundType.None;
			anyNonAir = false;
			
			// first check surrounding liquids/gas for sounds
			TouchesAny( bounds, CheckSoundNonSolid );
			if( sndType != SoundType.None ) return;
			
			// then check block standing on
			byte blockUnder = (byte)BlockUnderFeet;
			SoundType typeUnder = game.BlockInfo.StepSounds[blockUnder];
			BlockCollideType collideType = game.BlockInfo.CollideType[blockUnder];
			if( collideType == BlockCollideType.Solid && typeUnder != SoundType.None ) {
				anyNonAir = true; sndType = typeUnder; return;
			}
			
			// then check all solid blocks at feet
			pos.Y -= 0.01f;
			bounds.Max.Y = bounds.Min.Y = pos.Y;
			TouchesAny( bounds, CheckSoundSolid );
		}
		
		bool CheckSoundNonSolid( byte b ) {
			SoundType newType = game.BlockInfo.StepSounds[b];
			BlockCollideType collide = game.BlockInfo.CollideType[b];
			if( newType != SoundType.None && collide != BlockCollideType.Solid )
				sndType = newType;
			if( b != 0 ) anyNonAir = true;
			return false;
		}
		
		bool CheckSoundSolid( byte b ) {
			SoundType newType = game.BlockInfo.StepSounds[b];
			if( newType != SoundType.None ) sndType = newType;
			if( b != 0 ) anyNonAir = true;
			return false;
		}
		
		public override void RenderModel( double deltaTime, float t ) {
			anim.GetCurrentAnimState( t );
			curSwing = Utils.Lerp( anim.swingO, anim.swingN, t );
			curWalkTime = Utils.Lerp( anim.walkTimeO, anim.walkTimeN, t );
			
			if( !game.Camera.IsThirdPerson ) return;
			Model.Render( this );
		}
		
		public override void RenderName() {
			if( !game.Camera.IsThirdPerson ) return;
			DrawName();
		}
		
		void HandleInput( ref float xMoving, ref float zMoving ) {
			if( game.ScreenLockedInput ) {
				jumping = speeding = flyingUp = flyingDown = false;
			} else {
				if( game.IsKeyDown( KeyBinding.Forward ) ) xMoving -= 0.98f;
				if( game.IsKeyDown( KeyBinding.Back ) ) xMoving += 0.98f;
				if( game.IsKeyDown( KeyBinding.Left ) ) zMoving -= 0.98f;
				if( game.IsKeyDown( KeyBinding.Right ) ) zMoving += 0.98f;

				jumping = game.IsKeyDown( KeyBinding.Jump );
				speeding = Hacks.Enabled && game.IsKeyDown( KeyBinding.Speed );
				halfSpeeding = Hacks.Enabled && game.IsKeyDown( KeyBinding.HalfSpeed );
				flyingUp = game.IsKeyDown( KeyBinding.FlyUp );
				flyingDown = game.IsKeyDown( KeyBinding.FlyDown );
			}
		}
		
		internal bool jumping, speeding, halfSpeeding, flying, noClip, flyingDown, flyingUp;
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			if( !Hacks.CanFly || !Hacks.Enabled ) { flying = false; flyingDown = false; flyingUp = false; }
			if( !Hacks.CanNoclip || !Hacks.Enabled ) noClip = false;
			if( !Hacks.CanSpeed || !Hacks.Enabled ) { speeding = false; halfSpeeding = false; }
			Hacks.CanDoubleJump = Hacks.CanAnyHacks && Hacks.Enabled && Hacks.CanSpeed;
			
			if( !Hacks.CanUseThirdPersonCamera || !Hacks.Enabled )
				game.CycleCamera();
			if( !Hacks.Enabled || !Hacks.CanAnyHacks || !Hacks.CanSpeed )
				jumpVel = serverJumpVel;
		}
		
		internal Vector3 lastPos, nextPos;
		internal float lastYaw, nextYaw, lastPitch, nextPitch;
		float newYaw, oldYaw;
		int yawStateCount;
		float[] yawStates = new float[15];
		
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
					HeadYawDegrees = YawDegrees;
					yawStateCount = 0;
				} else {
					for( int i = 0; i < 3; i++ )
						AddYaw( Utils.LerpAngle( lastYaw, nextYaw, (i + 1) / 3f ) );
				}
			}
		}
		
		/// <summary> Linearly interpolates position and rotation between the previous and next state. </summary>
		public void SetInterpPosition( float t ) {
			Position = Vector3.Lerp( lastPos, nextPos, t );
			HeadYawDegrees = Utils.LerpAngle( lastYaw, nextYaw, t );
			YawDegrees = Utils.LerpAngle( oldYaw, newYaw, t );
			PitchDegrees = Utils.LerpAngle( lastPitch, nextPitch, t );
		}
		
		void AddYaw( float state ) {
			if( yawStateCount == yawStates.Length )
				RemoveOldest( yawStates, ref yawStateCount );
			yawStates[yawStateCount++] = state;
		}
		
		void UpdateCurrentBodyYaw() {
			oldYaw = newYaw;
			if( yawStateCount > 0 ) {
				newYaw = yawStates[0];
				RemoveOldest( yawStates, ref yawStateCount );
			}
		}
		
		void RemoveOldest<T>(T[] array, ref int count) {
			for( int i = 0; i < array.Length - 1; i++ )
				array[i] = array[i + 1];
			count--;
		}
		
		internal bool HandleKeyDown( Key key ) {
			KeyMap keys = game.InputHandler.Keys;
			if( key == keys[KeyBinding.Respawn] && Hacks.CanRespawn ) {
				Vector3I p = Vector3I.Floor( SpawnPoint );
				if( game.Map.IsValidPos( p ) ) {
					// Spawn player at highest valid position.
					for( int y = p.Y; y <= game.Map.Height; y++ ) {
						byte block1 = physics.GetPhysicsBlockId( p.X, y, p.Z );
						byte block2 = physics.GetPhysicsBlockId( p.X, y + 1, p.Z );
						if( info.CollideType[block1] != BlockCollideType.Solid &&
						   info.CollideType[block2] != BlockCollideType.Solid ) {
							p.Y = y;
							break;
						}
					}
				}
				Vector3 spawn = (Vector3)p + new Vector3( 0.5f, 0.01f, 0.5f );
				LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, SpawnYaw, SpawnPitch, false );
				SetLocation( update, false );
			} else if( key == keys[KeyBinding.SetSpawn] && Hacks.CanRespawn ) {
				SpawnPoint = Position;
				SpawnYaw = HeadYawDegrees;
				SpawnPitch = PitchDegrees;
			} else if( key == keys[KeyBinding.Fly] && Hacks.CanFly && Hacks.Enabled ) {
				flying = !flying;
			} else if( key == keys[KeyBinding.NoClip] && Hacks.CanNoclip && Hacks.Enabled ) {
				if( noClip ) Velocity.Y = 0;
				noClip = !noClip;
			} else if( key == keys[KeyBinding.Jump] && !onGround && !(flying || noClip) ) {
				if( !firstJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					DoNormalJump();
					firstJump = true;
				} else if( !secondJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					DoNormalJump();
					secondJump = true;
				}
			} else {
				return false;
			}
			return true;
		}
	}
}