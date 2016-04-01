// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	public partial class LocalPlayer : Player {
		
		/// <summary> Position the player's position is set to when the 'respawn' key binding is pressed. </summary>
		public Vector3 SpawnPoint;

		public float SpawnYaw, SpawnPitch;
		
		/// <summary> The distance (in blocks) that players are allowed to
		/// reach to and interact/modify blocks in. </summary>
		public float ReachDistance = 5f;
		
		/// <summary> Returns the height that the client can currently jump up to.<br/>
		/// Note that when speeding is enabled the client is able to jump much further. </summary>
		public float JumpHeight {
			get { return (float)PhysicsComponent.GetMaxHeight( physics.jumpVel ); }
		}
		
		internal float curWalkTime, curSwing;
		internal CollisionsComponent collisions;
		public HacksComponent Hacks;
		internal PhysicsComponent physics;
		
		public LocalPlayer( Game game ) : base( game ) {
			DisplayName = game.Username;
			SkinName = game.Username;
			SkinIdentifier = "skin_255";
			collisions = new CollisionsComponent( game, this );
			Hacks = new HacksComponent( game, this );
			physics = new PhysicsComponent( game, this );
			physics.hacks = Hacks;
			physics.collisions = collisions;
			
			Hacks.SpeedMultiplier = Options.GetFloat( OptionsKey.Speed, 0.1f, 50, 10 );
			Hacks.PushbackPlacing = !game.ClassicMode && Options.GetBool( OptionsKey.PushbackPlacing, false );
			Hacks.NoclipSlide = Options.GetBool( OptionsKey.NoclipSlide, false );
			Hacks.DoubleJump = !game.ClassicMode && Options.GetBool( OptionsKey.DoubleJump, false );
			Hacks.Enabled = !game.ClassicMode && Options.GetBool( OptionsKey.HacksEnabled, true );
			if( game.ClassicMode && game.ClassicHacks ) Hacks.Enabled = true;
			InitRenderingData();
		}
		
		Vector3 lastSoundPos = new Vector3( float.PositiveInfinity );
		public override void Tick( double delta ) {
			if( game.World.IsNotLoaded ) return;
			
			float xMoving = 0, zMoving = 0;
			lastPos = Position = nextPos;
			lastYaw = nextYaw;
			lastPitch = nextPitch;
			bool wasOnGround = onGround;
			
			HandleInput( ref xMoving, ref zMoving );
			physics.UpdateVelocityState( xMoving, zMoving );
			physics.PhysicsTick( xMoving, zMoving );
			
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
			CollideType collideType = game.BlockInfo.Collide[blockUnder];
			if( collideType == CollideType.Solid && typeUnder != SoundType.None ) {
				anyNonAir = true; sndType = typeUnder; return;
			}
			
			// then check all solid blocks at feet
			pos.Y -= 0.01f;
			bounds.Max.Y = bounds.Min.Y = pos.Y;
			TouchesAny( bounds, CheckSoundSolid );
		}
		
		bool CheckSoundNonSolid( byte b ) {
			SoundType newType = game.BlockInfo.StepSounds[b];
			CollideType collide = game.BlockInfo.Collide[b];
			if( newType != SoundType.None && collide != CollideType.Solid )
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
				physics.jumping = Hacks.Speeding = Hacks.FlyingUp = Hacks.FlyingDown = false;
			} else {
				if( game.IsKeyDown( KeyBinding.Forward ) ) xMoving -= 0.98f;
				if( game.IsKeyDown( KeyBinding.Back ) ) xMoving += 0.98f;
				if( game.IsKeyDown( KeyBinding.Left ) ) zMoving -= 0.98f;
				if( game.IsKeyDown( KeyBinding.Right ) ) zMoving += 0.98f;

				physics.jumping = game.IsKeyDown( KeyBinding.Jump );
				Hacks.Speeding = Hacks.Enabled && game.IsKeyDown( KeyBinding.Speed );
				Hacks.HalfSpeeding = Hacks.Enabled && game.IsKeyDown( KeyBinding.HalfSpeed );
				Hacks.FlyingUp = game.IsKeyDown( KeyBinding.FlyUp );
				Hacks.FlyingDown = game.IsKeyDown( KeyBinding.FlyDown );
			}
		}
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			Hacks.CheckHacksConsistency();
			if( !Hacks.Enabled || !Hacks.CanAnyHacks || !Hacks.CanSpeed )
				physics.jumpVel = physics.serverJumpVel;
		}
		
		internal Vector3 lastPos, nextPos;
		internal float lastYaw, nextYaw, lastPitch, nextPitch;
		float newYaw, oldYaw;
		int yawStateCount;
		float[] yawStates = new float[15];
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) {
			if( update.IncludesPosition ) {
				nextPos = update.RelativePosition ? nextPos + update.Pos : update.Pos;
				double blockOffset = nextPos.Y - Math.Floor( nextPos.Y );
				if( blockOffset < Entity.Adjustment )
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
				Vector3 spawn = SpawnPoint;
				if( game.World.IsValidPos( Vector3I.Floor( spawn ) ) )
					FindHighestFree( ref spawn );
				LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, SpawnYaw, SpawnPitch, false );
				SetLocation( update, false );
			} else if( key == keys[KeyBinding.SetSpawn] && Hacks.CanRespawn ) {
				SpawnPoint = Position;
				SpawnYaw = HeadYawDegrees;
				SpawnPitch = PitchDegrees;
			} else if( key == keys[KeyBinding.Fly] && Hacks.CanFly && Hacks.Enabled ) {
				Hacks.Flying = !Hacks.Flying;
			} else if( key == keys[KeyBinding.NoClip] && Hacks.CanNoclip && Hacks.Enabled ) {
				if( Hacks.Noclip ) Velocity.Y = 0;
				Hacks.Noclip = !Hacks.Noclip;
			} else if( key == keys[KeyBinding.Jump] && !onGround && !(Hacks.Flying || Hacks.Noclip) ) {
				if( !physics.firstJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					physics.DoNormalJump();
					physics.firstJump = true;
				} else if( !physics.secondJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					physics.DoNormalJump();
					physics.secondJump = true;
				}
			} else {
				return false;
			}
			return true;
		}
		
		void FindHighestFree( ref Vector3 spawn ) {
			Vector3 size = Model.CollisionSize;
			BoundingBox bb = CollisionBounds;
			Vector3I P = Vector3I.Floor( spawn );
			int bbMax = Utils.Floor( size.Y );			
			int minX = Utils.Floor( -size.X / 2 ), maxX = Utils.Floor( size.X / 2 );
			int minZ = Utils.Floor( -size.Z / 2 ), maxZ = Utils.Floor( size.Z / 2 );
			
			// Spawn player at highest valid position.
			for( int y = P.Y; y <= game.World.Height; y++ ) {
				bool anyHit = false;
				for( int yy = 0; yy <= bbMax; yy++ )
					for( int zz = minZ; zz <= maxZ; zz++ )
						for ( int xx = minX; xx <= maxX; xx++ )							
				{
					Vector3I coords = new Vector3I( P.X + xx, y + yy, P.Z + zz );
					byte block = collisions.GetPhysicsBlockId( coords.X, coords.Y, coords.Z );
					Vector3 min = info.MinBB[block] + (Vector3)coords;
					Vector3 max = info.MaxBB[block] + (Vector3)coords;
					if( !bb.Intersects( new BoundingBox( min, max ) ) ) continue;
					anyHit |= info.Collide[block] == CollideType.Solid;
				}
				
				if( !anyHit ) {
					byte block = collisions.GetPhysicsBlockId( P.X, y, P.Z );
					spawn.Y = y + info.MaxBB[block].Y + Entity.Adjustment;
					return;
				}
			}
		}
	}
}