// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp.Events;
using ClassicalSharp.Renderers;
using OpenTK.Input;

namespace ClassicalSharp.Commands {
	
	public sealed class ModelCommand : Command {
		
		public ModelCommand() {
			Name = "Model";
			Help = new [] {
				"&a/client model [name]",
				"&bnames: &echibi, chicken, creeper, human, pig, sheep",
				"&e       skeleton, spider, zombie, <numerical block id>",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string name = reader.Next();
			if( String.IsNullOrEmpty( name ) ) {
				game.Chat.Add( "&e/client model: &cYou didn't specify a model name." );
			} else {
				game.LocalPlayer.SetModel( Utils.ToLower( name ) );
			}
		}
	}
	
	public sealed class CuboidCommand : Command {
		
		public CuboidCommand() {
			Name = "Cuboid";
			Help = new [] {
				"&a/client cuboid [block] [persist]",
				"&eFills the 3D rectangle between two points with [block].",
				"&eIf no block is given, uses your currently held block.",
				"&e  If persist is given and is \"yes\", then the command",
				"&e  will repeatedly cuboid, without needing to be typed in again.",
			};
		}
		byte block = 0xFF;
		Vector3I mark1, mark2;
		bool persist = false;
		
		public override void Execute( CommandReader reader ) {
			game.UserEvents.BlockChanged -= BlockChanged;
			block = 0xFF;
			mark1 = new Vector3I( int.MaxValue );
			mark2 = new Vector3I( int.MaxValue );
			persist = false;
			
			if( !ParseBlock( reader ) ) return;
			string arg = reader.Next();
			if( arg != null && Utils.CaselessEquals( arg, "yes" ) )
				persist = true;
			
			game.Chat.Add( "&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3 );
			game.UserEvents.BlockChanged += BlockChanged;
		}
		
		bool ParseBlock( CommandReader reader ) {
			string id = reader.Next();
			if( id == null ) return true;
			if( Utils.CaselessEquals( id, "yes" ) ) { persist = true; return true; }
			
			byte blockID = 0;
			if( !byte.TryParse( id, out blockID ) ) {
				game.Chat.Add( "&eCuboid: &c\"" + id + "\" is not a valid block id." ); return false;
			}
			if( blockID >= Block.CpeCount && game.BlockInfo.Name[blockID] == "Invalid" ) {
				game.Chat.Add( "&eCuboid: &cThere is no block with id \"" + id + "\"." ); return false;
			}
			block = blockID;
			return true;
		}

		void BlockChanged( object sender, BlockChangedEventArgs e ) {
			if( mark1.X == int.MaxValue ) {
				mark1 = e.Coords;
				game.UpdateBlock( mark1.X, mark1.Y, mark1.Z, e.OldBlock );
				game.Chat.Add( "&eCuboid: &fMark 1 placed at (" + e.Coords + "), place mark 2.",
				              MessageType.ClientStatus3 );
			} else {
				mark2 = e.Coords;				
				DoCuboid();		
				game.Chat.Add( null, MessageType.ClientStatus3 );
				
				if( !persist ) {
					game.UserEvents.BlockChanged -= BlockChanged;
				} else {
					mark1 = new Vector3I( int.MaxValue );
					game.Chat.Add( "&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3 );
				}
			}
		}
		
		void DoCuboid() {
			Vector3I min = Vector3I.Min( mark1, mark2 );
			Vector3I max = Vector3I.Max( mark1, mark2 );
			if( !game.World.IsValidPos( min ) || !game.World.IsValidPos( max ) ) return;
			byte id = block;
			
			if( id == 0xFF ) id = (byte)game.Inventory.HeldBlock;		
			for( int y = min.Y; y <= max.Y; y++ )
				for( int z = min.Z; z <= max.Z; z++ )
					for( int x = min.X; x <= max.X; x++ ) 
			{
				game.UpdateBlock( x, y, z, id );
			}
		}
	}	
}
