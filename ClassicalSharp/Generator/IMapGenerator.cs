// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Map;
using ClassicalSharp.Singleplayer;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Generator {
	
	public abstract class IMapGenerator {
		
		public abstract string GeneratorName { get; }
		
		/// <summary> Generates the raw blocks within the map, using the given seed. </summary>
		public abstract BlockRaw[] Generate();
		
		/// <summary> Applies environment settings (if required) to the newly generated world. </summary>
		public virtual void ApplyEnv(World world) { }
		
		/// <summary> The current operation being performed  (current stage). </summary>
		public string CurrentState;
		
		/// <summary> Progress towards completion of the current operation. (raises from 0 to 1) </summary>
		public float CurrentProgress;
		
		/// <summary> Whether the generation has completed all operations. </summary>
		public bool Done = false;
		
		/// <summary> Blocks of the map generated. </summary>
		public volatile BlockRaw[] Blocks;
		
		public int Width, Height, Length, Seed;
		
		public void GenerateAsync(Game game) {
			Thread thread = new Thread(DoGenerate);
			thread.IsBackground = true;
			thread.Name = "IMapGenerator.GenAsync()";
			thread.Start();
		}
		
		void DoGenerate() {
			try {
				Blocks = Generate();
			} catch (Exception ex) {
				ErrorHandler.LogError("IMapGenerator.RunAsync", ex);
			}
			Done = true;
		}
	}
}
