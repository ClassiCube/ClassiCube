// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using ClassicalSharp.Entities;
using ClassicalSharp.Map;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Textures;
using OpenTK.Input;
using BlockID = System.UInt16;

namespace ClassicalSharp.Gui.Screens {
	public sealed class LoadLevelScreen : ListScreen {
		
		public LoadLevelScreen(Game game) : base(game) {
			titleText = "Select a level";
			string[] rawFiles = Platform.DirectoryFiles("maps");
			int count = 0;
			
			// Only add map files
			for (int i = 0; i < rawFiles.Length; i++) {
				string file = rawFiles[i];
				if (file.EndsWith(".cw") || file.EndsWith(".dat") || file.EndsWith(".fcm") || file.EndsWith(".lvl")) {
					count++;
				} else {
					rawFiles[i] = null;
				}
			}
			
			entries = new string[count];
			for (int i = 0, j = 0; i < rawFiles.Length; i++) {
				if (rawFiles[i] == null) continue;
				entries[j] = rawFiles[i]; j++;
			}
			Array.Sort(entries);
		}
		
		protected override void TextButtonClick(Game game, Widget widget) {
			string file = GetCur(widget);
			string path = Path.Combine("maps", file);
			if (!Platform.FileExists(path)) return;
			
			IMapFormatImporter importer = null;
			if (path.EndsWith(".dat")) {
				importer = new MapDatImporter();
			} else if (path.EndsWith(".fcm")) {
				importer = new MapFcm3Importer();
			} else if (path.EndsWith(".cw")) {
				importer = new MapCwImporter();
			} else if (path.EndsWith(".lvl")) {
				importer = new MapLvlImporter();
			}
			
			try {
				using (Stream fs = Platform.FileOpen(path)) {
					int width, height, length;
					game.World.Reset();
					game.WorldEvents.RaiseOnNewMap();
					
					if (game.World.TextureUrl != null) {
						TexturePack.ExtractDefault(game);
						game.World.TextureUrl = null;
					}
					BlockInfo.Reset();
					game.Inventory.SetDefaultMapping();
					
					byte[] blocks = importer.Load(fs, game, out width, out height, out length);
					game.World.SetNewMap(blocks, width, height, length);
					
					game.WorldEvents.RaiseOnNewMapLoaded();
					if (game.AllowServerTextures && game.World.TextureUrl != null)
						game.Server.RetrieveTexturePack(game.World.TextureUrl);
					
					LocalPlayer p = game.LocalPlayer;
					LocationUpdate update = LocationUpdate.MakePosAndOri(p.Spawn, p.SpawnRotY, p.SpawnHeadX, false);
					p.SetLocation(update, false);
				}
			} catch (Exception ex) {
				ErrorHandler.LogError("loading map", ex);
				game.Chat.Add("&eFailed to load map \"" + file + "\"");
			}
		}
	}
}