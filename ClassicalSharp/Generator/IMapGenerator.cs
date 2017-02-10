// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Map;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Generator {
	
	public abstract class IMapGenerator {
		
		public abstract string GeneratorName { get; }
		
		/// <summary> Generates the raw blocks within the map, using the given seed. </summary>
		public abstract byte[] Generate(int seed);
		
		/// <summary> Applies environment settings (if required) to the newly generated world. </summary>
		public virtual void ApplyEnv(World world) { }
		
		
		public string CurrentState;
		
		public float CurrentProgress;
		
		public bool Done = false;
		
		public int Width, Height, Length;	
		
		public void GenerateAsync(Game game, int width, int height, int length, int seed) {
			Width = width; Height = height; Length = length;
			Thread thread = new Thread(
				() => {
					SinglePlayerServer server = (SinglePlayerServer)game.Server;
					try {
						server.generatedMap = Generate(seed);
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
