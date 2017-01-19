// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK.Input;

namespace ClassicalSharp.Mode {
	
	public sealed class SurvivalGameMode : IGameMode {
		
		Game game;
		int score = 0;
		byte[] invCount = new byte[] { 0, 0, 0, 0, 0, 0, 0, 0, 10 };
		
		public bool HandlesKeyDown(Key key) { return false; }
		
		public void OnNewMapLoaded(Game game) {
			game.Chat.Add("&fScore: &e" + score, MessageType.Status1);
		}		
		
		public void Init(Game game) { 
			this.game = game;
			for (int i = 0; i < game.Inventory.Hotbar.Length; i++)
				game.Inventory.Hotbar[i] = Block.Invalid;
			game.Inventory.Hotbar[8] = Block.TNT;
		}
		
		
		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }	
		public void Dispose() { }
	}
}
