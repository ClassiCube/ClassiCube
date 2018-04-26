// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Physics;
using ClassicalSharp.Renderers;
using OpenTK;
using OpenTK.Input;
using BlockID = System.UInt16;

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
		internal SoundComponent sound;
		internal LocalInterpComponent interp;
		internal TiltComponent tilt;
		
		public LocalPlayer(Game game) : base(game) {
			DisplayName = game.Username;
			SkinName = game.Username;
			
			collisions = new CollisionsComponent(game, this);
			Hacks = new HacksComponent(game);
			physics = new PhysicsComponent(game, this);
			sound = new SoundComponent(game, this);
			interp = new LocalInterpComponent(game, this);
			tilt = new TiltComponent(game);		
			physics.hacks = Hacks; physics.collisions = collisions;
		}
		
		
		public override void SetLocation(LocationUpdate update, bool interpolate) {
			interp.SetLocation(update, interpolate);
		}
		
		public override void Tick(double delta) {
			if (!game.World.HasBlocks) return;
			StepSize = Hacks.FullBlockStep && Hacks.Enabled && Hacks.CanAnyHacks && Hacks.CanSpeed ? 1 : 0.5f;
			OldVelocity = Velocity;
			float xMoving = 0, zMoving = 0;
			interp.AdvanceState();
			bool wasOnGround = onGround;
			
			HandleInput(ref xMoving, ref zMoving);
			Hacks.Floating = Hacks.Noclip || Hacks.Flying;
			if (!Hacks.Floating && Hacks.CanBePushed) physics.DoEntityPush();
			
			// Immediate stop in noclip mode
			if (!Hacks.NoclipSlide && (Hacks.Noclip && xMoving == 0 && zMoving == 0)) {
				Velocity = Vector3.Zero;
			}
			
			physics.UpdateVelocityState();
			Vector3 headingVelocity = Utils.RotateY(xMoving, 0, zMoving, HeadYRadians);
			physics.PhysicsTick(headingVelocity);
			
			interp.next.Pos = Position; Position = interp.prev.Pos;
			anim.UpdateAnimState(interp.prev.Pos, interp.next.Pos, delta);
			tilt.UpdateAnimState(delta);
			
			CheckSkin();
			sound.Tick(wasOnGround);
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

		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			Hacks.CheckHacksConsistency();
			if (!Hacks.CanJumpHigher) {
				physics.jumpVel = physics.serverJumpVel;
			}
		}
		
		/// <summary> Linearly interpolates position and rotation between the previous and next state. </summary>
		public void SetInterpPosition(float t) {
			if (!(Hacks.WOMStyleHacks && Hacks.Noclip)) {
				Position = Vector3.Lerp(interp.prev.Pos, interp.next.Pos, t);
			}
			interp.LerpAngles(t);
		}
		
		void HandleInput(ref float xMoving, ref float zMoving) {
			if (game.Gui.ActiveScreen.HandlesAllInput) {
				physics.jumping = Hacks.Speeding = Hacks.FlyingUp = Hacks.FlyingDown = false;
			} else {
				if (game.IsKeyDown(KeyBind.Forward)) zMoving -= 0.98f;
				if (game.IsKeyDown(KeyBind.Back))    zMoving += 0.98f;
				if (game.IsKeyDown(KeyBind.Left))    xMoving -= 0.98f;
				if (game.IsKeyDown(KeyBind.Right))   xMoving += 0.98f;

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

		
		void IGameComponent.Init(Game game) {
			Hacks.Enabled = !game.PureClassic && Options.GetBool(OptionsKey.HacksOn, true);
			Health = 20;
			if (game.ClassicMode) return;
			
			Hacks.SpeedMultiplier = Options.GetFloat(OptionsKey.Speed, 0.1f, 50, 10);
			Hacks.PushbackPlacing = Options.GetBool(OptionsKey.PushbackPlacing, false);
			Hacks.NoclipSlide     = Options.GetBool(OptionsKey.NoclipSlide, false);
			Hacks.WOMStyleHacks   = Options.GetBool(OptionsKey.WOMStyleHacks, false);			
			Hacks.FullBlockStep   = Options.GetBool(OptionsKey.FullBlockStep, false);			
			physics.userJumpVel   = Options.GetFloat(OptionsKey.JumpVelocity, 0.0f, 52.0f, 0.42f);
			physics.jumpVel = physics.userJumpVel;			
		}
		
		void IGameComponent.Ready(Game game) { }
		void IGameComponent.OnNewMapLoaded(Game game) { }
		void IDisposable.Dispose() { }
		void IGameComponent.OnNewMap(Game game) { }
		
		void IGameComponent.Reset(Game game) {
			ReachDistance = 5;
			Velocity = Vector3.Zero;
			physics.jumpVel = 0.42f;
			physics.serverJumpVel = 0.42f;
			Health = 20;
		}
		
		
		static Predicate<BlockID> touchesAnySolid = IsSolidCollide;
		static bool IsSolidCollide(BlockID b) { return BlockInfo.Collide[b] == CollideType.Solid; }		
		void DoRespawn() {
			if (!game.World.HasBlocks) return;
			Vector3 spawn = Spawn;
			Vector3I P = Vector3I.Floor(spawn);
			AABB bb;
			
			// Spawn player at highest valid position
			if (game.World.IsValidPos(P)) {
				bb = AABB.Make(spawn, Size);
				for (int y = P.Y; y <= game.World.Height; y++) {
					float spawnY = Respawn.HighestFreeY(game, ref bb);
					if (spawnY == float.NegativeInfinity) {
						BlockID block = game.World.GetPhysicsBlock(P.X, y, P.Z);
						float height = BlockInfo.Collide[block] == CollideType.Solid ? BlockInfo.MaxBB[block].Y : 0;
						spawn.Y = y + height + Entity.Adjustment;
						break;
					}
					bb.Min.Y += 1; bb.Max.Y += 1;
				}
			}
			
			spawn.Y += 2/16f;
			LocationUpdate update = LocationUpdate.MakePosAndOri(spawn, SpawnRotY, SpawnHeadX, false);
			SetLocation(update, false);
			Velocity = Vector3.Zero;
			
			// Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before
			bb = Bounds;
			bb.Min.Y -= 0.01f; bb.Max.Y = bb.Min.Y;
			onGround = TouchesAny(bb, touchesAnySolid);
		}
		
		public bool HandlesKey(Key key) {
			KeyMap keys = game.Input.Keys;		
			if (key == keys[KeyBind.Respawn] && Hacks.CanRespawn) {
				DoRespawn();
			} else if (key == keys[KeyBind.SetSpawn] && Hacks.CanRespawn) {
				Spawn   = Position;
				Spawn.X = Utils.Floor(Spawn.X) + 0.5f;
				Spawn.Z = Utils.Floor(Spawn.Z) + 0.5f;
				SpawnRotY  = RotY;
				SpawnHeadX = HeadX;
				DoRespawn();
			} else if (key == keys[KeyBind.Fly] && Hacks.CanFly && Hacks.Enabled) {
				Hacks.Flying = !Hacks.Flying;
			} else if (key == keys[KeyBind.NoClip] && Hacks.CanNoclip && Hacks.Enabled && !Hacks.WOMStyleHacks) {
				if (Hacks.Noclip) Velocity.Y = 0;
				Hacks.Noclip = !Hacks.Noclip;
			} else if (key == keys[KeyBind.Jump] && !onGround && !(Hacks.Flying || Hacks.Noclip)) {
				int maxJumps = Hacks.CanDoubleJump && Hacks.WOMStyleHacks ? 2 : 0;
				maxJumps = Math.Max(maxJumps, Hacks.MaxJumps - 1);
				
				if (physics.multiJumps < maxJumps) {
					physics.DoNormalJump();
					physics.multiJumps++;
				}
			} else {
				return false;
			}
			return true;
		}
	}
}