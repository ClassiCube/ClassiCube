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
		
		/// <summary> The speed that the player move at, relative to normal speed,
		/// when the 'speeding' key binding is held down. </summary>
		public float SpeedMultiplier = 10;
		
		public byte UserType;
		
		/// <summary> Whether blocks that the player places that intersect themselves
		/// should cause the player to be pushed back in the opposite direction of the placed block. </summary>
		public bool PushbackPlacing;
		
		/// <summary> Whether the player has allowed hacks usage as an option.
		/// Note that all 'can use X' set by the server override this. </summary>
		public bool HacksEnabled = true;
		
		/// <summary> Whether the player is allowed to use any type of hacks. </summary>
		public bool CanAnyHacks = true;
		
		/// <summary> Whether the player is allowed to use the types of cameras that use third person. </summary>
		public bool CanUseThirdPersonCamera = true;
		
		/// <summary> Whether the player is allowed to increase its speed beyond the normal walking speed. </summary>
		public bool CanSpeed = true;
		
		/// <summary> Whether the player is allowed to fly in the world. </summary>
		public bool CanFly = true;
		
		/// <summary> Whether the player is allowed to teleport to their respawn coordinates. </summary>
		public bool CanRespawn = true;
		
		/// <summary> Whether the player is allowed to pass through all blocks. </summary>
		public bool CanNoclip  = true;
		
		/// <summary> Whether the player is allowed to use pushback block placing. </summary>
		public bool CanPushbackBlocks = true;
		
		/// <summary> Whether the player is allowed to see all entity name tags. </summary>
		public bool CanSeeAllNames = true;
		
		/// <summary> Whether the player should slide after letting go of movement buttons in noclip.  </summary>
		public bool NoclipSlide = true;
		
		/// <summary> Whether the player has allowed the usage of fast double jumping abilities. </summary>
		public bool DoubleJump = false;
		
		/// <summary> Whether the player is allowed to double jump. </summary>
		public bool CanDoubleJump = true;
		
		internal float jumpVel = 0.42f, serverJumpVel = 0.42f;
		/// <summary> Returns the height that the client can currently jump up to.<br/>
		/// Note that when speeding is enabled the client is able to jump much further. </summary>
		public float JumpHeight {
			get { return (float)GetMaxHeight( jumpVel ); }
		}
		
		internal float curWalkTime, curSwing;
		
		public LocalPlayer( Game game ) : base( game ) {
			DisplayName = game.Username;
			SkinName = game.Username;
			SkinIdentifier = "skin_255";
			
			SpeedMultiplier = Options.GetFloat( OptionsKey.Speed, 0.1f, 50, 7 );
			PushbackPlacing = Options.GetBool( OptionsKey.PushbackPlacing, false );
			NoclipSlide = Options.GetBool( OptionsKey.NoclipSlide, false );
			DoubleJump = Options.GetBool( OptionsKey.DoubleJump, false );
			HacksEnabled = Options.GetBool( OptionsKey.HacksEnabled, true );
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
			UpdateAnimState( lastPos, nextPos, delta );
			CheckSkin();
			
			Vector3 soundPos = nextPos;
			bool anyNonAir = false;
			SoundType type = GetSound( ref anyNonAir );
			if( !anyNonAir ) soundPos = new Vector3( -100000 );
			
			if( onGround && (DoPlaySound( soundPos ) || !wasOnGround) ) {
				game.AudioPlayer.PlayStepSound( type );
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
			float oldLegRot = (float)Math.Sin( walkTimeO );
			float newLegRot = (float)Math.Sin( walkTimeN );
			return Math.Sign( oldLegRot ) != Math.Sign( newLegRot );
		}
		
		SoundType GetSound( ref bool anyNonAir ) {
			Vector3 pos = nextPos, size = CollisionSize;
			BoundingBox bounds = new BoundingBox( pos - size / 2, pos + size / 2);
			SoundType type = SoundType.None;
			bool nonAir = false;
			
			// first check surrounding liquids/gas for sounds
			TouchesAny( bounds, b => {
			           	SoundType newType = game.BlockInfo.StepSounds[b];
			           	BlockCollideType collide = game.BlockInfo.CollideType[b];
			           	if( newType != SoundType.None && collide != BlockCollideType.Solid)
			           		type = newType;
			           	if( b != 0 ) nonAir = true;
			           	return false;
			           });
			if( type != SoundType.None )
				return type;
			
			// then check block standing on
			byte blockUnder = (byte)BlockUnderFeet;
			SoundType typeUnder = game.BlockInfo.StepSounds[blockUnder];
			BlockCollideType collideType = game.BlockInfo.CollideType[blockUnder];
			if( collideType == BlockCollideType.Solid && typeUnder != SoundType.None ) {
				nonAir = true;
				return typeUnder;
			}
			
			// then check all solid blocks at feet
			pos.Y -= 0.01f;
			bounds.Max.Y = bounds.Min.Y = pos.Y;
			TouchesAny( bounds, b => {
			           	SoundType newType = game.BlockInfo.StepSounds[b];
			           	if( newType != SoundType.None ) type = newType;
			           	if( b != 0 ) nonAir = true;
			           	return false;
			           });
			anyNonAir = nonAir;
			return type;
		}
		
		public override void RenderModel( double deltaTime, float t ) {
			GetCurrentAnimState( t );
			curSwing = Utils.Lerp( swingO, swingN, t );
			curWalkTime = Utils.Lerp( walkTimeO, walkTimeN, t );
			
			if( !game.Camera.IsThirdPerson ) return;
			Model.RenderModel( this );
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
				speeding = CanSpeed && HacksEnabled && game.IsKeyDown( KeyBinding.Speed );
				halfSpeeding = CanSpeed && HacksEnabled && game.IsKeyDown( KeyBinding.HalfSpeed );
				flyingUp = game.IsKeyDown( KeyBinding.FlyUp );
				flyingDown = game.IsKeyDown( KeyBinding.FlyDown );
			}
		}
		
		internal bool jumping, speeding, halfSpeeding, flying, noClip, flyingDown, flyingUp;
		
		/// <summary> Parses hack flags specified in the motd and/or name of the server. </summary>
		/// <remarks> Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax </remarks>
		public void ParseHackFlags( string name, string motd ) {
			string joined = name + motd;
			SetAllHacks( true );
			// By default (this is also the case with WoM), we can use hacks.
			if( joined.Contains( "-hax" ) )
				SetAllHacks( false );
			
			ParseFlag( b => CanFly = b, joined, "fly" );
			ParseFlag( b => CanNoclip = b, joined, "noclip" );
			ParseFlag( b => CanSpeed = b, joined, "speed" );
			ParseFlag( b => CanRespawn = b, joined, "respawn" );

			if( UserType == 0x64 )
				ParseFlag( b => SetAllHacks( b ), joined, "ophax" );
			CheckHacksConsistency();
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void SetAllHacks( bool allowed ) {
			CanAnyHacks = CanFly = CanNoclip = CanRespawn = CanSpeed =
				CanPushbackBlocks = CanUseThirdPersonCamera = allowed;
		}
		
		static void ParseFlag( Action<bool> action, string joined, string flag ) {
			if( joined.Contains( "+" + flag ) )
				action( true );
			else if( joined.Contains( "-" + flag ) )
				action( false );
		}
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			if( !CanFly || !HacksEnabled ) { flying = false; flyingDown = false; flyingUp = false; }
			if( !CanNoclip || !HacksEnabled ) noClip = false;
			if( !CanSpeed || !HacksEnabled ) { speeding = false; halfSpeeding = false; }
			if( !CanPushbackBlocks || !HacksEnabled ) PushbackPlacing = false;
			CanDoubleJump = CanAnyHacks && HacksEnabled && CanSpeed;
			
			if( !CanUseThirdPersonCamera || !HacksEnabled )
				game.CycleCamera();
			if( !HacksEnabled || !CanAnyHacks || !CanSpeed )
				jumpVel = serverJumpVel;
		}
		
		/// <summary> Sets the user type of this user. This is used to control permissions for grass,
		/// bedrock, water and lava blocks on servers that don't support CPE block permissions. </summary>
		public void SetUserType( byte value ) {
			UserType = value;
			Inventory inv = game.Inventory;
			inv.CanPlace[(int)Block.Bedrock] = value == 0x64;
			inv.CanDelete[(int)Block.Bedrock] = value == 0x64;

			inv.CanPlace[(int)Block.Water] = value == 0x64;
			inv.CanPlace[(int)Block.StillWater] = value == 0x64;
			inv.CanPlace[(int)Block.Lava] = value == 0x64;
			inv.CanPlace[(int)Block.StillLava] = value == 0x64;
			CanSeeAllNames = value == 0x64;
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
		
		internal bool HandleKeyDown( Key key ) {
			KeyMap keys = game.InputHandler.Keys;
			if( key == keys[KeyBinding.Respawn] && CanRespawn && HacksEnabled ) {
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
				LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, SpawnYaw, SpawnPitch, false );
				SetLocation( update, false );
			} else if( key == keys[KeyBinding.SetSpawn] && CanRespawn && HacksEnabled ) {
				SpawnPoint = Position;
				SpawnYaw = HeadYawDegrees;
				SpawnPitch = PitchDegrees;
			} else if( key == keys[KeyBinding.Fly] && CanFly && HacksEnabled ) {
				flying = !flying;
			} else if( key == keys[KeyBinding.NoClip] && CanNoclip && HacksEnabled ) {
				noClip = !noClip;
			} else if( key == keys[KeyBinding.Jump] && !onGround && !(flying || noClip) ) {
				if( !firstJump && CanDoubleJump && DoubleJump ) {
					DoNormalJump();
					firstJump = true;
				} else if( !secondJump && CanDoubleJump && DoubleJump ) {
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