// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Screens;
using OpenTK.Input;

namespace ClassicalSharp.Mode {
	
	public sealed class CreativeGameMode : IGameMode {
		
		Game game;
		
		public bool HandlesKeyDown(Key key) {
			if (key == game.Input.Keys[KeyBind.Inventory]) {
				game.Gui.SetNewScreen(new InventoryScreen(game));
				return true;
			}
			return false;
		}
		
		public void OnNewMapLoaded(Game game) {
			if (game.Server.IsSinglePlayer)
				game.Chat.Add("&ePlaying single player", MessageType.Status1);
		}

		public void Init(Game game) {
			this.game = game;
			game.Inventory.Hotbar = new byte[] { Block.Stone,
				Block.Cobblestone, Block.Brick, Block.Dirt, Block.Wood,
				Block.Log, Block.Leaves, Block.Grass, Block.Slab };
		}
		
		
		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void Dispose() { }
	}
}
