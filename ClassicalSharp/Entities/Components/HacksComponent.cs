// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {

	/// <summary> Entity component that performs management of hack states. </summary>
	public sealed class HacksComponent {
		
		Game game;
		public HacksComponent(Game game, Entity entity) {
			this.game = game;
		}
		
		public byte UserType;
		
		/// <summary> The speed that the player move at, relative to normal speed,
		/// when the 'speeding' key binding is held down. </summary>
		public float SpeedMultiplier = 10;
		
		/// <summary> Whether blocks that the player places that intersect themselves
		/// should cause the player to be pushed back in the opposite direction of the placed block. </summary>
		public bool PushbackPlacing;
		
		/// <summary> Whether the player should be able to step up whole blocks, instead of just slabs. </summary>
		public bool FullBlockStep;
		
		/// <summary> Whether the player has allowed hacks usage as an option.
		/// Note that all 'can use X' set by the server override this. </summary>
		public bool Enabled = true;
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
		public bool CanNoclip = true;
		/// <summary> Whether the player is allowed to use pushback block placing. </summary>
		public bool CanPushbackBlocks = true;
		/// <summary> Whether the player is allowed to see all entity name tags. </summary>
		public bool CanSeeAllNames = true;
		/// <summary> Whether the player is allowed to double jump. </summary>
		public bool CanDoubleJump = true;
		/// <summary> Whether the player can be pushed by other players. </summary>
		public bool CanBePushed = true;
		/// <summary> Base speed multiplier entity moves at horizontally. </summary>
		public float BaseHorSpeed = 1;
		/// <summary> Amount of jumps the player can perform. </summary>
		public int MaxJumps = 1;
		
		/// <summary> Whether the player should slide after letting go of movement buttons in noclip.  </summary>
		public bool NoclipSlide = true;
		/// <summary> Whether the player has allowed the usage of fast double jumping abilities. </summary>
		public bool WOMStyleHacks;
		
		/// <summary> Whether the player currently has noclip on. </summary>
		public bool Noclip;
		/// <summary> Whether the player currently has fly mode active. </summary>
		public bool Flying;
		/// <summary> Whether the player is currently flying upwards. </summary>
		public bool FlyingUp;
		/// <summary> Whether the player is currently flying downwards. </summary>
		public bool FlyingDown;
		/// <summary> Whether the player is currently walking at base speed * speed multiplier. </summary>
		public bool Speeding;
		/// <summary> Whether the player is currently walking at base speed * 0.5 * speed multiplier. </summary>
		public bool HalfSpeeding;
		
		public bool CanJumpHigher { get { return Enabled && CanAnyHacks && CanSpeed; } }
		public bool Floating { get { return Noclip || Flying; } }
		public string HacksFlags;
		
		string GetFlagValue(string flag) {
			int start = HacksFlags.IndexOf(flag, StringComparison.OrdinalIgnoreCase);
			if (start < 0) return null;
			start += flag.Length;
			
			int end = HacksFlags.IndexOf(' ', start);
			if (end < 0) end = HacksFlags.Length;
			return HacksFlags.Substring(start, end - start);
		}
		
		void ParseHorizontalSpeed() {
			string num = GetFlagValue("horspeed=");
			if (num == null) return;
			
			float value = 0;
			if (!Utils.TryParseDecimal(num, out value) || value <= 0) return;
			BaseHorSpeed = value;
		}
		
		void ParseMultiJumps() {
			string num = GetFlagValue("jumps=");
			if (num == null || game.ClassicMode) return;
			
			int value = 0;
			if (!int.TryParse(num, out value) || value < 0) return;
			MaxJumps = value;
		}
		
		void SetAllHacks(bool allowed) {
			CanAnyHacks = CanFly = CanNoclip = CanRespawn = CanSpeed =
				CanPushbackBlocks = CanUseThirdPersonCamera = allowed;
		}
		
		void ParseFlag(ref bool target, string flag) {
			if (HacksFlags.Contains("+" + flag)) {
				target = true;
			} else if (HacksFlags.Contains("-" + flag)) {
				target = false;
			}
		}
		
		void ParseAllFlag(string flag) {
			if (HacksFlags.Contains("+" + flag)) {
				SetAllHacks(true);
			} else if (HacksFlags.Contains("-" + flag)) {
				SetAllHacks(false);
			}
		}
		
		/// <summary> Sets the user type of this user. This is used to control permissions for grass,
		/// bedrock, water and lava blocks on servers that don't support CPE block permissions. </summary>
		public void SetUserType(byte value) {
			bool isOp = value >= 100 && value <= 127;
			UserType = value;
			BlockInfo.CanPlace[Block.Bedrock] = isOp;
			BlockInfo.CanDelete[Block.Bedrock] = isOp;

			BlockInfo.CanPlace[Block.Water] = isOp;
			BlockInfo.CanPlace[Block.StillWater] = isOp;
			BlockInfo.CanPlace[Block.Lava] = isOp;
			BlockInfo.CanPlace[Block.StillLava] = isOp;
			CanSeeAllNames = isOp;
		}
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			if (!CanFly || !Enabled) { Flying = false; FlyingDown = false; FlyingUp = false; }
			if (!CanNoclip || !Enabled) Noclip = false;
			if (!CanSpeed || !Enabled) { Speeding = false; HalfSpeeding = false; }
			CanDoubleJump = CanAnyHacks && Enabled && CanSpeed;
			CanSeeAllNames = CanAnyHacks && CanSeeAllNames;
			
			if (!CanUseThirdPersonCamera || !Enabled)
				game.CycleCamera();
		}
		
		
		/// <summary> Updates ability to use hacks, and raises HackPermissionsChanged event. </summary>
		/// <remarks> Parses hack flags specified in the motd and/or name of the server. </remarks>
		/// <remarks> Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz </remarks>
		public void UpdateHacksState() {
			SetAllHacks(true);
			if (HacksFlags == null) return;
			
			BaseHorSpeed = 1;
			MaxJumps = 1;
			CanBePushed = true;
			
			// By default (this is also the case with WoM), we can use hacks.
			if (HacksFlags.Contains("-hax")) SetAllHacks(false);
			
			ParseFlag(ref CanFly, "fly");
			ParseFlag(ref CanNoclip, "noclip");
			ParseFlag(ref CanSpeed, "speed");
			ParseFlag(ref CanRespawn, "respawn");
			ParseFlag(ref CanBePushed, "push");

			if (UserType == 0x64) ParseAllFlag("ophax");
			ParseHorizontalSpeed();
			ParseMultiJumps();
			
			CheckHacksConsistency();
			game.Events.RaiseHackPermissionsChanged();
		}
	}
}
