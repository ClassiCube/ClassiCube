// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	public partial class LocalPlayer : Player {
		
		/// <summary> Position the player's position is set to when the 'respawn' key binding is pressed. </summary>
		public Vector3 Spawn;

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
		internal InputComponent input;
		internal SoundComponent sound;
		
		public LocalPlayer( Game game ) : base( game ) {
			DisplayName = game.Username;
			SkinName = game.Username;
			SkinIdentifier = "skin_255";
			
			collisions = new CollisionsComponent( game, this );
			Hacks = new HacksComponent( game, this );
			physics = new PhysicsComponent( game, this );
			input = new InputComponent( game, this );
			sound = new SoundComponent( game, this );
			
			physics.hacks = Hacks; input.Hacks = Hacks;
			physics.collisions = collisions; input.collisions = collisions;
			input.physics = physics;
			
			Hacks.SpeedMultiplier = Options.GetFloat( OptionsKey.Speed, 0.1f, 50, 10 );
			Hacks.PushbackPlacing = !game.ClassicMode && Options.GetBool( OptionsKey.PushbackPlacing, false );
			Hacks.NoclipSlide = Options.GetBool( OptionsKey.NoclipSlide, false );
			Hacks.DoubleJump = !game.ClassicMode && Options.GetBool( OptionsKey.DoubleJump, false );
			Hacks.Enabled = !game.ClassicMode && Options.GetBool( OptionsKey.HacksEnabled, true );
			Hacks.FullBlockStep = !game.ClassicMode && Options.GetBool( OptionsKey.FullBlockStep, false );
			if( game.ClassicMode && game.ClassicHacks ) Hacks.Enabled = true;
		}
		
		public override void Tick( double delta ) {
			if( game.World.IsNotLoaded ) return;
			StepSize = Hacks.FullBlockStep && Hacks.Enabled && Hacks.CanAnyHacks 
				&& Hacks.CanSpeed ? 1 : 0.5f;
			
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
			sound.Tick( wasOnGround );
			UpdateCurrentBodyYaw();
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
			if( game.ActiveScreen.HandlesAllInput ) {
				physics.jumping = Hacks.Speeding = Hacks.FlyingUp = Hacks.FlyingDown = false;
			} else {
				if( game.IsKeyDown( KeyBind.Forward ) ) xMoving -= 0.98f;
				if( game.IsKeyDown( KeyBind.Back ) ) xMoving += 0.98f;
				if( game.IsKeyDown( KeyBind.Left ) ) zMoving -= 0.98f;
				if( game.IsKeyDown( KeyBind.Right ) ) zMoving += 0.98f;

				physics.jumping = game.IsKeyDown( KeyBind.Jump );
				Hacks.Speeding = Hacks.Enabled && game.IsKeyDown( KeyBind.Speed );
				Hacks.HalfSpeeding = Hacks.Enabled && game.IsKeyDown( KeyBind.HalfSpeed );
				Hacks.FlyingUp = game.IsKeyDown( KeyBind.FlyUp );
				Hacks.FlyingDown = game.IsKeyDown( KeyBind.FlyDown );
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
		
		internal bool HandleKeyDown( Key key ) { return input.HandleKeyDown( key ); }
	}
}