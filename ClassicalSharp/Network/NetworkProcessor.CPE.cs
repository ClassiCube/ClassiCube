// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Hotkeys;
using ClassicalSharp.Map;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : INetworkProcessor {
		
		#region Writing
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
			Player p = game.LocalPlayer;
			MakePlayerClick( (byte)button, buttonDown, p.HeadYawDegrees, p.PitchDegrees, targetId,
			                pos.BlockPos, pos.BlockFace );
			SendPacket();
		}
		
		void MakeExtInfo( string appName, int extensionsCount ) {
			writer.WriteUInt8( (byte)Opcode.CpeExtInfo );
			writer.WriteString( appName );
			writer.WriteInt16( (short)extensionsCount );
		}
		
		void MakeExtEntry( string extensionName, int extensionVersion ) {
			writer.WriteUInt8( (byte)Opcode.CpeExtEntry );
			writer.WriteString( extensionName );
			writer.WriteInt32( extensionVersion );
		}
		
		void MakeCustomBlockSupportLevel( byte version ) {
			writer.WriteUInt8( (byte)Opcode.CpeCustomBlockSupportLevel );
			writer.WriteUInt8( version );
		}
		
		void MakePlayerClick( byte button, bool buttonDown, float yaw, float pitch, byte targetEntity,
		                     Vector3I targetPos, CpeBlockFace targetFace ) {
			writer.WriteUInt8( (byte)Opcode.CpePlayerClick );
			writer.WriteUInt8( button );
			writer.WriteUInt8( buttonDown ? (byte)0 : (byte)1 );
			writer.WriteInt16( (short)Utils.DegreesToPacked( yaw, 65536 ) );
			writer.WriteInt16( (short)Utils.DegreesToPacked( pitch, 65536 ) );
			writer.WriteUInt8( targetEntity );
			writer.WriteInt16( (short)targetPos.X );
			writer.WriteInt16( (short)targetPos.Y );
			writer.WriteInt16( (short)targetPos.Z );
			writer.WriteUInt8( (byte)targetFace );
		}
		
		#endregion
		
		
		#region Reading
		
		internal void HandleExtInfo() {
			string appName = reader.ReadAsciiString();
			game.Chat.Add( "Server software: " + appName );
			if( Utils.CaselessStarts( appName, "D3 server" ) )
				cpe.needD3Fix = true;
			
			// Workaround for MCGalaxy that send ExtEntry sync but ExtInfoAsync. This means
			// ExtEntry may sometimes arrive before ExtInfo, and thus we have to use += instead of =
			cpe.ServerExtensionsCount += reader.ReadInt16();
			SendCpeExtInfoReply();
		}
		
		internal void HandleExtEntry() {
			string extName = reader.ReadAsciiString();
			int extVersion = reader.ReadInt32();
			Utils.LogDebug( "cpe ext: {0}, {1}", extName, extVersion );
			cpe.HandleEntry( extName, extVersion, this );
			SendCpeExtInfoReply();
		}
		
		void SendCpeExtInfoReply() {
			if( cpe.ServerExtensionsCount != 0 ) return;
			string[] clientExts = CPESupport.ClientExtensions;
			int count = clientExts.Length;
			if( !game.AllowCustomBlocks ) count -= 2;
			
			MakeExtInfo( Program.AppName, count );
			SendPacket();
			for( int i = 0; i < clientExts.Length; i++ ) {
				string name = clientExts[i];
				int ver = 1;
				if( name == "ExtPlayerList" ) ver = 2;
				if( name == "EnvMapAppearance" ) ver = cpe.envMapVer;
				if( name == "BlockDefinitionsExt" ) ver = cpe.blockDefsExtVer;
				
				if( !game.AllowCustomBlocks && name.StartsWith( "BlockDefinitions" ) )
					continue;
				MakeExtEntry( name, ver );
				SendPacket();
			}
		}
		
		internal void HandleSetClickDistance() {
			game.LocalPlayer.ReachDistance = reader.ReadInt16() / 32f;
		}
		
		internal void HandleCustomBlockSupportLevel() {
			byte supportLevel = reader.ReadUInt8();
			MakeCustomBlockSupportLevel( 1 );
			SendPacket();
			game.UseCPEBlocks = true;

			if( supportLevel == 1 ) {
				for( int i = (int)Block.CobblestoneSlab; i <= (int)Block.StoneBrick; i++ ) {
					game.Inventory.CanPlace[i] = true;
					game.Inventory.CanDelete[i] = true;
				}
				game.Events.RaiseBlockPermissionsChanged();
			} else {
				Utils.LogDebug( "Server's block support level is {0}, this client only supports level 1.", supportLevel );
				Utils.LogDebug( "You won't be able to see or use blocks from levels above level 1" );
			}
		}
		
		internal void HandleHoldThis() {
			byte blockType = reader.ReadUInt8();
			bool canChange = reader.ReadUInt8() == 0;
			game.Inventory.CanChangeHeldBlock = true;
			game.Inventory.HeldBlock = (Block)blockType;
			game.Inventory.CanChangeHeldBlock = canChange;
		}
		
		internal void HandleSetTextHotkey() {
			string label = reader.ReadAsciiString();
			string action = reader.ReadCp437String();
			int keyCode = reader.ReadInt32();
			byte keyMods = reader.ReadUInt8();
			
			if( keyCode < 0 || keyCode > 255 ) return;
			Key key = LwjglToKey.Map[keyCode];
			if( key == Key.Unknown ) return;
			
			Utils.LogDebug( "CPE Hotkey added: " + key + "," + keyMods + " : " + action );
			if( action == "" ) {
				game.InputHandler.Hotkeys.RemoveHotkey( key, keyMods );
			} else if( action[action.Length - 1] == '\n' ) {
				action = action.Substring( 0, action.Length - 1 );
				game.InputHandler.Hotkeys.AddHotkey( key, keyMods, action, false );
			} else { // more input needed by user
				game.InputHandler.Hotkeys.AddHotkey( key, keyMods, action, true );
			}
		}
		
		internal void HandleExtAddPlayerName() {
			short id = reader.ReadInt16();
			string playerName = Utils.StripColours( reader.ReadAsciiString() );
			playerName = Utils.RemoveEndPlus( playerName );
			string listName = reader.ReadAsciiString();
			listName = Utils.RemoveEndPlus( listName );
			string groupName = reader.ReadAsciiString();
			byte groupRank = reader.ReadUInt8();
			
			// Workaround for some servers that don't cast signed bytes to unsigned, before converting them to shorts.
			if( id < 0 ) id += 256;
			if( id >= 0 && id <= 255 )
				AddTablistEntry( (byte)id, playerName, listName, groupName, groupRank );
		}
		
		void AddTablistEntry( byte id, string playerName, string listName, string groupName, byte groupRank ) {
			TabListEntry oldInfo = game.TabList.Entries[id];
			TabListEntry info = new TabListEntry( (byte)id, playerName, listName, groupName, groupRank );
			game.TabList.Entries[id] = info;
			
			if( oldInfo != null ) {
				// Only redraw the tab list if something changed.
				if( info.PlayerName != oldInfo.PlayerName || info.ListName != oldInfo.ListName ||
				   info.GroupName != oldInfo.GroupName || info.GroupRank != oldInfo.GroupRank ) {
					game.EntityEvents.RaiseTabListEntryChanged( id );
				}
			} else {
				game.EntityEvents.RaiseTabEntryAdded( id );
			}
		}
		
		internal void HandleExtAddEntity() {
			byte id = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			displayName = Utils.RemoveEndPlus( displayName );
			string skinName = reader.ReadAsciiString();
			skinName = Utils.RemoveEndPlus( skinName );
			AddEntity( id, displayName, skinName, false );
		}
		
		internal void HandleExtRemovePlayerName() {
			short id = reader.ReadInt16();
			// Workaround for some servers that don't cast signed bytes to unsigned, before converting them to shorts.
			if( id < 0 ) id += 256;
			
			if( id >= 0 && id <= 255 ) {
				game.EntityEvents.RaiseTabEntryRemoved( (byte)id );
				game.TabList.Entries[id] = null;
			}
		}
		
		internal void HandleMakeSelection() {
			byte selectionId = reader.ReadUInt8();
			string label = reader.ReadAsciiString();
			short startX = reader.ReadInt16();
			short startY = reader.ReadInt16();
			short startZ = reader.ReadInt16();
			short endX = reader.ReadInt16();
			short endY = reader.ReadInt16();
			short endZ = reader.ReadInt16();
			
			byte r = (byte)reader.ReadInt16();
			byte g = (byte)reader.ReadInt16();
			byte b = (byte)reader.ReadInt16();
			byte a = (byte)reader.ReadInt16();
			
			Vector3I p1 = Vector3I.Min( startX, startY, startZ, endX, endY, endZ );
			Vector3I p2 = Vector3I.Max( startX, startY, startZ, endX, endY, endZ );
			FastColour col = new FastColour( r, g, b, a );
			game.SelectionManager.AddSelection( selectionId, p1, p2, col );
		}
		
		internal void HandleRemoveSelection() {
			byte selectionId = reader.ReadUInt8();
			game.SelectionManager.RemoveSelection( selectionId );
		}
		
		internal void HandleEnvColours() {
			byte variable = reader.ReadUInt8();
			short red = reader.ReadInt16();
			short green = reader.ReadInt16();
			short blue = reader.ReadInt16();
			bool invalid = red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255;
			FastColour col = new FastColour( red, green, blue );

			if( variable == 0 ) {
				game.World.Env.SetSkyColour( invalid ? WorldEnv.DefaultSkyColour : col );
			} else if( variable == 1 ) {
				game.World.Env.SetCloudsColour( invalid ? WorldEnv.DefaultCloudsColour : col );
			} else if( variable == 2 ) {
				game.World.Env.SetFogColour( invalid ? WorldEnv.DefaultFogColour : col );
			} else if( variable == 3 ) {
				game.World.Env.SetShadowlight( invalid ? WorldEnv.DefaultShadowlight : col );
			} else if( variable == 4 ) {
				game.World.Env.SetSunlight( invalid ? WorldEnv.DefaultSunlight : col );
			}
		}
		
		internal void HandleSetBlockPermission() {
			byte blockId = reader.ReadUInt8();
			bool canPlace = reader.ReadUInt8() != 0;
			bool canDelete = reader.ReadUInt8() != 0;
			Inventory inv = game.Inventory;
			
			if( blockId == 0 ) {
				int count = game.UseCPEBlocks ? BlockInfo.CpeCount
					: BlockInfo.OriginalCount;
				for( int i = 1; i < count; i++ ) {
					inv.CanPlace.SetNotOverridable( canPlace, i );
					inv.CanDelete.SetNotOverridable( canDelete, i );
				}
			} else {
				inv.CanPlace.SetNotOverridable( canPlace, blockId );
				inv.CanDelete.SetNotOverridable( canDelete, blockId );
			}
			game.Events.RaiseBlockPermissionsChanged();
		}
		
		internal void HandleChangeModel() {
			byte playerId = reader.ReadUInt8();
			string modelName = reader.ReadAsciiString().ToLowerInvariant();
			Player player = game.Entities[playerId];
			if( player != null ) player.SetModel( modelName );
		}
		
		internal void HandleEnvSetMapAppearance() {
			HandleSetMapEnvUrl();
			game.World.Env.SetSidesBlock( (Block)reader.ReadUInt8() );
			game.World.Env.SetEdgeBlock( (Block)reader.ReadUInt8() );
			game.World.Env.SetEdgeLevel( reader.ReadInt16() );
		}
		
		internal void HandleEnvSetMapAppearance2() {
			HandleEnvSetMapAppearance();
			game.World.Env.SetCloudsLevel( reader.ReadInt16() );
			short maxViewDist = reader.ReadInt16();
			game.MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
			game.SetViewDistance( game.UserViewDistance, false );
		}
		
		internal void HandleEnvWeatherType() {
			game.World.Env.SetWeather( (Weather)reader.ReadUInt8() );
		}
		
		internal void HandleHackControl() {
			LocalPlayer p = game.LocalPlayer;
			p.Hacks.CanFly = reader.ReadUInt8() != 0;
			p.Hacks.CanNoclip = reader.ReadUInt8() != 0;
			p.Hacks.CanSpeed = reader.ReadUInt8() != 0;
			p.Hacks.CanRespawn = reader.ReadUInt8() != 0;
			p.Hacks.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
			p.CheckHacksConsistency();
			
			float jumpHeight = reader.ReadInt16() / 32f;
			if( jumpHeight < 0 ) p.physics.jumpVel = 0.42f;
			else p.physics.CalculateJumpVelocity( jumpHeight );
			p.physics.serverJumpVel = p.physics.jumpVel;
			game.Events.RaiseHackPermissionsChanged();
		}
		
		internal void HandleExtAddEntity2() {
			byte id = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			string skinName = reader.ReadAsciiString();
			AddEntity( id, displayName, skinName, true );
		}
		
		const int bulkCount = 256;
		unsafe void HandleBulkBlockUpdate() {
			int count = reader.ReadUInt8() + 1;
			if( game.World.IsNotLoaded ) {
				#if DEBUG_BLOCKS
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
				#endif
				reader.Skip( bulkCount * (sizeof(int) + 1) );
				return;
			}
			int* indices = stackalloc int[bulkCount];
			for( int i = 0; i < count; i++ )
				indices[i] = reader.ReadInt32();
			reader.Skip( (bulkCount - count) * sizeof(int) );
			
			for( int i = 0; i < count; i++ ) {
				byte block = reader.ReadUInt8();
				Vector3I coords = game.World.GetCoords( indices[i] );
				
				if( coords.X < 0 ) {
					#if DEBUG_BLOCKS
					Utils.LogDebug( "Server tried to update a block at an invalid position!" );
					#endif
					continue;
				}
				game.UpdateBlock( coords.X, coords.Y, coords.Z, block );
			}
			reader.Skip( bulkCount - count );
		}
		
		internal void HandleSetTextColor() {
			FastColour col = new FastColour( reader.ReadUInt8(), reader.ReadUInt8(),
			                                reader.ReadUInt8(), reader.ReadUInt8() );
			byte code = reader.ReadUInt8();
			
			if( code <= ' ' || code > '~' ) return; // Control chars, space, extended chars cannot be used
			if( (code >= '0' && code <= '9') || (code >= 'a' && code <= 'f')
			   || (code >= 'A' && code <= 'F') ) return; // Standard chars cannot be used.
			if( code == '%' || code == '&' ) return; // colour code signifiers cannot be used
			
			game.Drawer2D.Colours[code] = col;
			game.Events.RaiseColourCodesChanged();
		}
		
		internal void HandleSetMapEnvUrl() {
			string url = reader.ReadAsciiString();
			if( !game.AllowServerTextures ) return;
			
			if( url == "" ) {
				ExtractDefault();
			} else if( Utils.IsUrlPrefix( url, 0 ) ) {
				RetrieveTexturePack( url );
			}
			Utils.LogDebug( "Image url: " + url );
		}
		
		internal void HandleSetMapEnvProperty() {
			byte type = reader.ReadUInt8();
			int value = reader.ReadInt32();
			WorldEnv env = game.World.Env;
			Utils.Clamp( ref value, short.MinValue, short.MaxValue );
			
			switch( type ) {
				case 0:
					Utils.Clamp( ref value, byte.MinValue, byte.MaxValue );
					env.SetSidesBlock( (Block)value ); break;
				case 1:
					Utils.Clamp( ref value, byte.MinValue, byte.MaxValue );
					env.SetEdgeBlock( (Block)value ); break;
				case 2:
					env.SetEdgeLevel( value ); break;
				case 3:
					env.SetCloudsLevel( value ); break;
				case 4:
					game.MaxViewDistance = value <= 0 ? 32768 : value;
					game.SetViewDistance( game.UserViewDistance, false ); break;
				case 5:
					env.SetCloudsSpeed( value / 256f ); break;
				case 6:
					env.SetWeatherSpeed( value / 256f ); break;
				case 7:
					Utils.Clamp( ref value, byte.MinValue, byte.MaxValue );
					Console.WriteLine( value );
					env.SetWeatherFade( value / 128f ); break;
			}
		}
	}
	#endregion
}