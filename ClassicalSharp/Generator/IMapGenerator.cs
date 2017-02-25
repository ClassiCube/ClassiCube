// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Map;
using ClassicalSharp.Singleplayer;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Generator {
	
	public abstract class IMapGenerator {
		
		public abstract string GeneratorName { get; }
		
		/// <summary> Generates the raw blocks within the map, using the given seed. </summary>
		public abstract BlockID[] Generate(int seed);
		
		/// <summary> Applies environment settings (if required) to the newly generated world. </summary>
		public virtual void ApplyEnv(World world) { }
		
		/// <summary> The current operation being performed  (current stage). </summary>
		public string CurrentState;
		
		/// <summary> Progress towards completion of the current operation. (raises from 0 to 1) </summary>
		public float CurrentProgress;
		
		/// <summary> Whether the generation has completed all operations. </summary>
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
