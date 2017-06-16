// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.Renderers;
using OpenTK.Input;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Commands {
	
	public sealed class ModelCommand : Command {
		
		public ModelCommand() {
			Name = "Model";
			Help = new string[] {
				"&a/client model [name]",
				"&bnames: &echibi, chicken, creeper, human, pig, sheep",
				"&e       skeleton, spider, zombie, sitting, <numerical block id>",
			};
		}
		
		public override void Execute(string[] args) {
			if (args.Length == 1) {
				game.Chat.Add("&e/client model: &cYou didn't specify a model name.");
			} else {
				game.LocalPlayer.SetModel(Utils.ToLower(args[1]));
			}
		}
	}
	
	public sealed class CuboidCommand : Command {
		
		public CuboidCommand() {
			Name = "Cuboid";
			Help = new string[] {
				"&a/client cuboid [block] [persist]",
				"&eFills the 3D rectangle between two points with [block].",
				"&eIf no block is given, uses your currently held block.",
				"&e  If persist is given and is \"yes\", then the command",
				"&e  will repeatedly cuboid, without needing to be typed in again.",
			};
		}
		int block = -1;
		Vector3I mark1, mark2;
		bool persist = false;
		
		public override void Execute(string[] args) {
			game.UserEvents.BlockChanged -= BlockChanged;
			block = -1;
			mark1 = new Vector3I(int.MaxValue);
			mark2 = new Vector3I(int.MaxValue);
			persist = false;
			
			if (!ParseBlock(args)) return;
			if (args.Length > 2 && Utils.CaselessEquals(args[2], "yes"))
				persist = true;
			
			game.Chat.Add("&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3);
			game.UserEvents.BlockChanged += BlockChanged;
		}
		
		bool ParseBlock(string[] args) {
			if (args.Length == 1) return true;
			if (Utils.CaselessEquals(args[1], "yes")) { persist = true; return true; }
			
			int temp = -1;
			BlockID block = 0;
			if ((temp = game.BlockInfo.FindID(args[1])) != -1) {
				block = (BlockID)temp;
			} else if (!BlockID.TryParse(args[1], out block)) {
				game.Chat.Add("&eCuboid: &c\"" + args[1] + "\" is not a valid block name or id."); return false;
			}
			
			if (block >= Block.CpeCount && game.BlockInfo.Name[block] == "Invalid") {
				game.Chat.Add("&eCuboid: &cThere is no block with id \"" + args[1] + "\"."); return false;
			}
			this.block = block;
			return true;
		}

		void BlockChanged(object sender, BlockChangedEventArgs e) {
			if (mark1.X == int.MaxValue) {
				mark1 = e.Coords;
				game.UpdateBlock(mark1.X, mark1.Y, mark1.Z, e.OldBlock);
				game.Chat.Add("&eCuboid: &fMark 1 placed at (" + e.Coords + "), place mark 2.",
				              MessageType.ClientStatus3);
			} else {
				mark2 = e.Coords;				
				DoCuboid();		
				game.Chat.Add(null, MessageType.ClientStatus3);
				
				if (!persist) {
					game.UserEvents.BlockChanged -= BlockChanged;
				} else {
					mark1 = new Vector3I(int.MaxValue);
					game.Chat.Add("&eCuboid: &fPlace or delete a block.", MessageType.ClientStatus3);
				}
			}
		}
		
		void DoCuboid() {
			Vector3I min = Vector3I.Min(mark1, mark2);
			Vector3I max = Vector3I.Max(mark1, mark2);
			if (!game.World.IsValidPos(min) || !game.World.IsValidPos(max)) return;
			
			BlockID toPlace = (BlockID)block;
			if (block == -1) toPlace = game.Inventory.Selected;
			
			for (int y = min.Y; y <= max.Y; y++)
				for (int z = min.Z; z <= max.Z; z++)
					for (int x = min.X; x <= max.X; x++) 
			{
				game.UpdateBlock(x, y, z, toPlace);
			}
		}
	}	
	
	public sealed class TeleportCommand : Command {
		
		public TeleportCommand() {
			Name = "Teleport";
			Help = new string[] {
				"&a/client teleport [x y z]",
				"&eMoves you to the given coordinates.",
			};
		}
		
		public override void Execute(string[] args) {
			if (args.Length != 4) {
				game.Chat.Add("&e/client teleport: &cYou didn't specify X, Y and Z coordinates.");
			} else {
				float x = 0, y = 0, z = 0;
				if (!Utils.TryParseDecimal(args[1], out x) ||
				    !Utils.TryParseDecimal(args[2], out y) ||
				    !Utils.TryParseDecimal(args[3], out z)) {
					game.Chat.Add("&e/client teleport: &cCoordinates must be decimals");
				}
				
				Vector3 v = new Vector3(x, y, z);
				LocationUpdate update = LocationUpdate.MakePos(v, false);
				game.LocalPlayer.SetLocation(update, false);
			}
		}
	}	
}
