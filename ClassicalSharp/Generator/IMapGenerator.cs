﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Generator {
	
	public abstract class IMapGenerator {
		
		public abstract string GeneratorName { get; }
		
		public abstract byte[] Generate(Game game, int width, int height, int length, int seed, bool winter);
		
		public string CurrentState;
		
		public float CurrentProgress;
		
		public bool Done = false;
		
		public int Width, Height, Length;	
		
		public void GenerateAsync(Game game, int width, int height, int length, int seed, bool winter) {
			Width = width; Height = height; Length = length;
			Thread thread = new Thread(
				() => {
					SinglePlayerServer server = (SinglePlayerServer)game.Server;
					try {
						server.generatedMap = Generate(game, width, height, length, seed, winter);
					} catch (Exception ex) {
						ErrorHandler.LogError("IMapGenerator.RunAsync", ex);
					}
					Done = true;
				}
			);
			
			thread.IsBackground = true;
			thread.Name = "IMapGenerator.RunAsync";
			thread.Start();
		}
	}
}
