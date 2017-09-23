// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	public class LocalPlayer : Player, IGameComponent {
		
		/// <summary> Position the player's position is set to when the 'respawn' key binding is pressed. </summary>
		public Vector3 Spawn;

		public float SpawnRotY, SpawnHeadX;
		
		/// <summary> The distance (in blocks) that players are allowed to
		/// reach to and interact/modify blocks in. </summary>
		public float ReachDistance = 5f;
		
		/// <summary> Returns the height that the client can currently jump up to.<br/>
		/// Note that when speeding is enabled the client is able to jump much further. </summary>
		public float JumpHeight {
			get { return (float)PhysicsComponent.GetMaxHeight(physics.jumpVel); }
		}
		
		internal CollisionsComponent collisions;
		public HacksComponent Hacks;
		internal PhysicsComponent physics;
		internal InputComponent input;
		internal SoundComponent sound;
		internal LocalInterpComponent interp;
		internal TiltComponent tilt;
		
		public LocalPlayer(Game game) : base(game) {
			DisplayName = game.Username;
			SkinName = game.Username;
			
			collisions = new CollisionsComponent(game, this);
			Hacks = new HacksComponent(game, this);
			physics = new PhysicsComponent(game, this);
			input = new InputComponent(game, this);
			sound = new SoundComponent(game, this);
			interp = new LocalInterpComponent(game, this);
			tilt = new TiltComponent(game);
			
			physics.hacks = Hacks; input.Hacks = Hacks;
			physics.collisions = collisions;
			input.physics = physics;
		}
		
		public override void Tick(double delta) {
			if (game.World.blocks == null) return;
			StepSize = Hacks.FullBlockStep && Hacks.Enabled && Hacks.CanAnyHacks
				&& Hacks.CanSpeed ? 1 : 0.5f;
			OldVelocity = Velocity;
			float xMoving = 0, zMoving = 0;
			interp.AdvanceState();
			bool wasOnGround = onGround;
			
			HandleInput(ref xMoving, ref zMoving);
			if (!Hacks.Floating && Hacks.CanBePushed) physics.DoEntityPush();
			
			// Immediate stop in noclip mode
			if (!Hacks.NoclipSlide && (Hacks.Noclip && xMoving == 0 && zMoving == 0))
				Velocity = Vector3.Zero;
			physics.UpdateVelocityState();
			physics.PhysicsTick(GetHeadingVelocity(zMoving, xMoving));
			
			interp.next.Pos = Position; Position = interp.prev.Pos;
			anim.UpdateAnimState(interp.prev.Pos, interp.next.Pos, delta);
			tilt.UpdateAnimState(delta);
			
			CheckSkin();
			sound.Tick(wasOnGround);
		}
		
		Vector3 GetHeadingVelocity(float xMoving, float zMoving) {
			return Utils.RotateY(xMoving, 0, zMoving, HeadYRadians);
		}
		

		public override void RenderModel(double deltaTime, float t) {
			anim.GetCurrentAnimState(t);
			tilt.GetCurrentAnimState(t);
			
			if (!game.Camera.IsThirdPerson) return;
			Model.Render(this);
		}
		
		public override void RenderName() {
			if (!game.Camera.IsThirdPerson) return;
			DrawName();
		}
		
		void HandleInput(ref float xMoving, ref float zMoving) {
			if (game.Gui.ActiveScreen.HandlesAllInput) {
				physics.jumping = Hacks.Speeding = Hacks.FlyingUp = Hacks.FlyingDown = false;
			} else {
				if (game.IsKeyDown(KeyBind.Forward)) xMoving -= 0.98f;
				if (game.IsKeyDown(KeyBind.Back)) xMoving += 0.98f;
				if (game.IsKeyDown(KeyBind.Left)) zMoving -= 0.98f;
				if (game.IsKeyDown(KeyBind.Right)) zMoving += 0.98f;

				physics.jumping = game.IsKeyDown(KeyBind.Jump);
				Hacks.Speeding = Hacks.Enabled && game.IsKeyDown(KeyBind.Speed);
				Hacks.HalfSpeeding = Hacks.Enabled && game.IsKeyDown(KeyBind.HalfSpeed);
				Hacks.FlyingUp = game.IsKeyDown(KeyBind.FlyUp);
				Hacks.FlyingDown = game.IsKeyDown(KeyBind.FlyDown);
				
				if (Hacks.WOMStyleHacks && Hacks.Enabled && Hacks.CanNoclip) {
					if (Hacks.Noclip) Velocity = Vector3.Zero;
					Hacks.Noclip = game.IsKeyDown(KeyBind.NoClip);
				}
			}
		}
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			Hacks.CheckHacksConsistency();
			if (!Hacks.CanJumpHigher) {
				physics.jumpVel = physics.serverJumpVel;
			}
		}
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		/// <summary> Linearly interpolates position and rotation between the previous and next state. </summary>
		public void SetInterpPosition(float t) {
			if (!Hacks.WOMStyleHacks || !Hacks.Noclip)
				Position = Vector3.Lerp(interp.prev.Pos, interp.next.Pos, t);
			interp.LerpAngles(t);
		}
		
		public void Init(Game game) {
			Hacks.SpeedMultiplier = Options.GetFloat(OptionsKey.Speed, 0.1f, 50, 10);
			Hacks.PushbackPlacing = !game.ClassicMode && Options.GetBool(OptionsKey.PushbackPlacing, false);
			Hacks.NoclipSlide = Options.GetBool(OptionsKey.NoclipSlide, false);
			Hacks.WOMStyleHacks = !game.ClassicMode && Options.GetBool(OptionsKey.WOMStyleHacks, false);
			Hacks.Enabled = !game.PureClassic && Options.GetBool(OptionsKey.HacksOn, true);
			Hacks.FullBlockStep = !game.ClassicMode && Options.GetBool(OptionsKey.FullBlockStep, false);
			Health = 20;
		}
		
		public void Ready(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		public void Dispose() { }
		public void OnNewMap(Game game) { }
		
		public void Reset(Game game) {
			ReachDistance = 5;
			Velocity = Vector3.Zero;
			physics.jumpVel = 0.42f;
			physics.serverJumpVel = 0.42f;
			Health = 20;
		}
	}
}