// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Map;
using ClassicalSharp.Singleplayer;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Generator {
	
	public abstract class IMapGenerator {
		
		public abstract string GeneratorName { get; }

		public abstract BlockRaw[] Generate();

		public virtual void ApplyEnv(World world) { }

		public string CurrentState;

		public float CurrentProgress;

		public bool Done = false;

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
