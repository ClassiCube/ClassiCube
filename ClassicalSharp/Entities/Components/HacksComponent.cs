// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Entities {

	/// <summary> Entity component that performs management of hack states. </summary>
	public sealed class HacksComponent {
		
		Game game;
		Entity entity;
		public HacksComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
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
		/// <summary> Maximum speed the entity can move at horizontally when CanSpeed is false. </summary>
		public float MaxSpeedMultiplier = 1;
		
		/// <summary> Whether the player should slide after letting go of movement buttons in noclip.  </summary>
		public bool NoclipSlide = true;		
		/// <summary> Whether the player has allowed the usage of fast double jumping abilities. </summary>
		public bool DoubleJump = false;
		
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
		
		/// <summary> Parses hack flags specified in the motd and/or name of the server. </summary>
		/// <remarks> Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz </remarks>
		public void ParseHackFlags( string name, string motd ) {
			string joined = name + motd;
			SetAllHacks( true );
			MaxSpeedMultiplier = 1;
			// By default (this is also the case with WoM), we can use hacks.
			if( joined.Contains( "-hax" ) ) SetAllHacks( false );
			
			ParseFlag( b => CanFly = b, joined, "fly" );
			ParseFlag( b => CanNoclip = b, joined, "noclip" );
			ParseFlag( b => CanSpeed = b, joined, "speed" );
			ParseFlag( b => CanRespawn = b, joined, "respawn" );

			if( UserType == 0x64 )
				ParseFlag( b => SetAllHacks( b ), joined, "ophax" );
			ParseHorizontalSpeed( joined );
		}
		
		void ParseHorizontalSpeed( string joined ) {
			int start = joined.IndexOf( "horspeed=", StringComparison.OrdinalIgnoreCase );
			if( start < 0 ) return;
			start += 9;
			
			int end = joined.IndexOf(' ', start );
			if( end < 0 ) end = joined.Length;
			
			string num = joined.Substring( start, end - start );
			float value = 0;
			if( !Single.TryParse( num, out value ) || value <= 0 ) return;
			MaxSpeedMultiplier = value;
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
		
		/// <summary> Disables any hacks if their respective CanHackX value is set to false. </summary>
		public void CheckHacksConsistency() {
			if( !CanFly || !Enabled ) { Flying = false; FlyingDown = false; FlyingUp = false; }
			if( !CanNoclip || !Enabled ) Noclip = false;
			if( !CanSpeed || !Enabled ) { Speeding = false; HalfSpeeding = false; }
			CanDoubleJump = CanAnyHacks && Enabled && CanSpeed;
			
			if( !CanUseThirdPersonCamera || !Enabled )
				game.CycleCamera();
		}
	}
}